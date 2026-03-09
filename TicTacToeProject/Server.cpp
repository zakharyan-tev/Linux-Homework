#include "Server.h"
#include "Protocol.h"

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

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

    if (serverFd_ < 0)
        throw std::runtime_error("socket failed");

    setSocketOptions(serverFd_);

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port_);

    if (bind(serverFd_, (sockaddr*)&addr, sizeof(addr)) < 0)
        throw std::runtime_error("bind failed");

    if (listen(serverFd_, 64) < 0)
        throw std::runtime_error("listen failed");

    running_ = true;

    std::cout << "[Server] started on port "
              << port_
              << std::endl;

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
        socklen_t len = sizeof(clientAddr);

        int clientFd = accept(
            serverFd_,
            (sockaddr*)&clientAddr,
            &len
        );

        if (clientFd < 0)
            continue;

        std::thread(
            &Server::handleClient,
            this,
            clientFd
        ).detach();
    }
}

std::shared_ptr<GameSession>
Server::getOrCreateSession(int clientFd) {

    std::lock_guard<std::mutex> lock(pendingMutex_);

    if (!pendingSession_) {

        int sid = nextSessionId_++;

        auto session =
            std::make_shared<GameSession>(sid);

        {
            std::lock_guard<std::mutex> s(sessionsMutex_);
            sessions_[sid] = session;
        }

        session->addPlayer(clientFd);

        pendingSession_ = session;

        return session;
    }

    auto session = pendingSession_;

    session->addPlayer(clientFd);

    pendingSession_ = nullptr;

    return session;
}

void Server::handleClient(int clientFd) {

    auto session = getOrCreateSession(clientFd);

    {
        std::lock_guard<std::mutex> lock(clientMapMutex_);
        clientToSession_[clientFd] = session->sessionId;
    }

    GameSession::sendToPlayer(
        clientFd,
        "INFO TicTacToe Server\n"
    );

    char buf[512];
    std::string buffer;

    while (running_) {

        ssize_t n = recv(clientFd, buf, sizeof(buf)-1, 0);

        if (n <= 0) break;

        buf[n] = '\0';
        buffer += buf;

        size_t pos;

        while ((pos = buffer.find('\n')) != std::string::npos) {

            std::string line = buffer.substr(0,pos);
            buffer.erase(0,pos+1);

            processMessage(clientFd,line);
        }
    }

    cleanupClient(clientFd);
}

void Server::processMessage(
    int clientFd,
    const std::string& msg)
{

    std::string cmd = Protocol::parseCommand(msg);

    int sessionId = -1;

    {
        std::lock_guard<std::mutex> lock(clientMapMutex_);

        auto it = clientToSession_.find(clientFd);

        if (it != clientToSession_.end())
            sessionId = it->second;
    }

    std::shared_ptr<GameSession> session;

    {
        std::lock_guard<std::mutex> lock(sessionsMutex_);

        auto it = sessions_.find(sessionId);

        if (it != sessions_.end())
            session = it->second;
    }

    if (!session) return;

    if (cmd == "MOVE") {

        int pos = Protocol::parseMovePosition(msg);

        if (pos < 0 || pos > 8) {
            GameSession::sendToPlayer(
                clientFd,
                "MOVE_ERR Invalid position\n"
            );
            return;
        }

        session->handleMove(clientFd,pos);
    }

    else if (cmd == "QUIT") {

        int opp = session->getOpponent(clientFd);

        if (opp != -1)
            GameSession::sendToPlayer(
                opp,
                Protocol::DISCONNECTED + "\n"
            );
    }
}

void Server::cleanupClient(int clientFd) {

    close(clientFd);

    std::lock_guard<std::mutex> lock(clientMapMutex_);

    clientToSession_.erase(clientFd);
}
