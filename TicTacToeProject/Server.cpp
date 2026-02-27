#include "Server.h"
#include "Protocol.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <netinet/tcp.h>
#include <iostream>
#include <sstream>
#include <cstring>
#include <algorithm>

Server::Server(int port) : port_(port), serverFd_(-1) {}

Server::~Server() {
    stop();
}

void Server::setSocketOptions(int fd) {
    int opt = 1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &opt, sizeof(opt));
}

void Server::start() {
    serverFd_ = socket(AF_INET, SOCK_STREAM, 0);
    if (serverFd_ < 0) {
        throw std::runtime_error("Failed to create socket");
    }
    setSocketOptions(serverFd_);

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port_);

    if (bind(serverFd_, (sockaddr*)&addr, sizeof(addr)) < 0) {
        close(serverFd_);
        throw std::runtime_error("Bind failed: " + std::string(strerror(errno)));
    }

    if (listen(serverFd_, 64) < 0) {
        close(serverFd_);
        throw std::runtime_error("Listen failed");
    }

    running_ = true;
    std::cout << "╔══════════════════════════════════════╗\n";
    std::cout << "║   TicTacToe Multiplayer Server       ║\n";
    std::cout << "║   Listening on port " << port_ << "              ║\n";
    std::cout << "╚══════════════════════════════════════╝\n";
    std::cout << "[Server] Started. Waiting for connections...\n\n";

    acceptLoop();
}

void Server::stop() {
    running_ = false;
    if (serverFd_ >= 0) {
        close(serverFd_);
        serverFd_ = -1;
    }
}

void Server::acceptLoop() {
    while (running_) {
        sockaddr_in clientAddr{};
        socklen_t clientLen = sizeof(clientAddr);
        int clientFd = accept(serverFd_, (sockaddr*)&clientAddr, &clientLen);
        if (clientFd < 0) {
            if (running_) std::cerr << "[Server] Accept error: " << strerror(errno) << "\n";
            continue;
        }

        char ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &clientAddr.sin_addr, ip, sizeof(ip));
        std::cout << "[Server] New connection: fd=" << clientFd << " from " << ip << "\n";

        // Detach thread per client
        std::thread(&Server::handleClient, this, clientFd).detach();
    }
}

std::shared_ptr<GameSession> Server::getOrCreateSession(int clientFd) {
    std::lock_guard<std::mutex> lock(pendingMutex_);

    if (!pendingSession_) {
        // Create new session
        int sid = nextSessionId_++;
        auto session = std::make_shared<GameSession>(sid);
        {
            std::lock_guard<std::mutex> slock(sessionsMutex_);
            sessions_[sid] = session;
        }
        session->addPlayer(clientFd);
        pendingSession_ = session;
        std::cout << "[Server] Created session #" << sid << " for fd=" << clientFd << " (waiting for opponent)\n";
        return session;
    } else {
        // Join existing session
        auto session = pendingSession_;
        session->addPlayer(clientFd);
        pendingSession_ = nullptr;
        std::cout << "[Server] fd=" << clientFd << " joined session #" << session->sessionId << "\n";
        return session;
    }
}

void Server::handleClient(int clientFd) {
    // Assign to a session
    auto session = getOrCreateSession(clientFd);
    {
        std::lock_guard<std::mutex> lock(clientMapMutex_);
        clientToSession_[clientFd] = session->sessionId;
    }

    // Send greeting
    GameSession::sendToPlayer(clientFd,
        "INFO TicTacToe Server v1.0 | Commands: MOVE <0-8>, QUIT\n");

    // Read loop
    std::string buffer;
    char chunk[512];

    while (running_) {
        ssize_t n = recv(clientFd, chunk, sizeof(chunk) - 1, 0);
        if (n <= 0) break;

        chunk[n] = '\0';
        buffer += chunk;

        // Process line by line
        size_t pos;
        while ((pos = buffer.find('\n')) != std::string::npos) {
            std::string line = buffer.substr(0, pos);
            buffer.erase(0, pos + 1);

            // Trim carriage return
            if (!line.empty() && line.back() == '\r') line.pop_back();
            if (line.empty()) continue;

            std::cout << "[fd=" << clientFd << "] Received: " << line << "\n";
            processMessage(clientFd, line);
        }
    }

    cleanupClient(clientFd);
}

void Server::processMessage(int clientFd, const std::string& msg) {
    std::string cmd = Protocol::parseCommand(msg);

    // Get session
    int sessionId = -1;
    {
        std::lock_guard<std::mutex> lock(clientMapMutex_);
        auto it = clientToSession_.find(clientFd);
        if (it != clientToSession_.end()) sessionId = it->second;
    }

    std::shared_ptr<GameSession> session;
    if (sessionId != -1) {
        std::lock_guard<std::mutex> lock(sessionsMutex_);
        auto it = sessions_.find(sessionId);
        if (it != sessions_.end()) session = it->second;
    }

    if (!session) {
        GameSession::sendToPlayer(clientFd, "ERR Not in a session\n");
        return;
    }

    if (cmd == "MOVE") {
        int pos = Protocol::parseMovePosition(msg);
        if (pos < 0 || pos > 8) {
            GameSession::sendToPlayer(clientFd, "MOVE_ERR Position must be 0-8\n");
            return;
        }
        {
            // Check game state before move
            std::lock_guard<std::mutex> lock(session->stateMutex);
            if (session->state.status == GameStatus::WAITING) {
                GameSession::sendToPlayer(clientFd, "MOVE_ERR Waiting for opponent\n");
                return;
            }
            if (session->isOver()) {
                GameSession::sendToPlayer(clientFd, "MOVE_ERR Game is already over\n");
                return;
            }
        }
        session->handleMove(clientFd, pos);
    }
    else if (cmd == "QUIT") {
        GameSession::sendToPlayer(clientFd, "INFO Goodbye!\n");
        // notify opponent
        int opp = session->getOpponent(clientFd);
        if (opp != -1) GameSession::sendToPlayer(opp, Protocol::DISCONNECTED + "\n");
    }
    else {
        GameSession::sendToPlayer(clientFd, "ERR Unknown command. Use: MOVE <0-8>, QUIT\n");
    }
}

void Server::cleanupClient(int clientFd) {
    std::cout << "[Server] Client fd=" << clientFd << " disconnected\n";

    // Find session and notify opponent
    int sessionId = -1;
    {
        std::lock_guard<std::mutex> lock(clientMapMutex_);
        auto it = clientToSession_.find(clientFd);
        if (it != clientToSession_.end()) {
            sessionId = it->second;
            clientToSession_.erase(it);
        }
    }

    if (sessionId != -1) {
        std::shared_ptr<GameSession> session;
        {
            std::lock_guard<std::mutex> lock(sessionsMutex_);
            auto it = sessions_.find(sessionId);
            if (it != sessions_.end()) session = it->second;
        }
        if (session) {
            int opp = session->getOpponent(clientFd);
            if (opp != -1) {
                GameSession::sendToPlayer(opp, Protocol::DISCONNECTED + "\n");
            }
            // Remove pending session if it was this one
            {
                std::lock_guard<std::mutex> lock(pendingMutex_);
                if (pendingSession_ == session) pendingSession_ = nullptr;
            }
        }
    }

    close(clientFd);
}
