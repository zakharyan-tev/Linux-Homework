#include <iostream>
#include <string>
#include <thread>
#include <atomic>
#include <sstream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>

std::atomic<bool> running{true};
int sockFd = -1;

void receiveLoop() {
    char buf[2048];
    std::string buffer;

    while (running) {
        ssize_t n = recv(sockFd, buf, sizeof(buf) - 1, 0);
        if (n <= 0) {
            if (running) std::cout << "\n[Client] Disconnected from server.\n";
            running = false;
            break;
        }
        buf[n] = '\0';
        buffer += buf;

        size_t pos;
        while ((pos = buffer.find('\n')) != std::string::npos) {
            std::string line = buffer.substr(0, pos);
            buffer.erase(0, pos + 1);
            if (!line.empty() && line.back() == '\r') line.pop_back();
            if (line.empty()) continue;

            // Pretty-print server messages
            if (line.substr(0, 6) == "BOARD ") {
                std::cout << "\n" << line.substr(6) << "\n";
            } else if (line.substr(0, 7) == "RESULT ") {
                std::cout << "\n🏆 " << line.substr(7) << "\n";
                running = false;
            } else if (line.substr(0, 7) == "WELCOME") {
                std::cout << "✅ " << line << "\n";
            } else if (line.substr(0, 4) == "TURN") {
                std::cout << "🎮 " << line.substr(5) << "\n";
            } else if (line.substr(0, 10) == "GAME_START") {
                std::cout << "🎯 " << line.substr(11) << "\n";
            } else if (line.substr(0, 4) == "WAIT") {
                std::cout << "⏳ " << line.substr(8) << "\n";
            } else if (line.substr(0, 8) == "MOVE_ERR") {
                std::cout << "❌ " << line.substr(9) << "\n";
            } else if (line.substr(0, 7) == "MOVE_OK") {
                std::cout << "✔️  Move accepted at position " << line.substr(8) << "\n";
            } else if (line.substr(0, 5) == "STATE") {
                // STATE <board> <turn> <status>
                // Just show parsed info
                std::istringstream ss(line.substr(6));
                std::string board, turn, status;
                ss >> board >> turn >> status;
                std::cout << "[State] Turn=" << turn << " Status=" << status << "\n";
            } else if (line == "OPPONENT_DISCONNECTED") {
                std::cout << "⚠️  Opponent disconnected!\n";
                running = false;
            } else {
                std::cout << "📨 " << line << "\n";
            }
            std::cout << std::flush;
        }
    }
}

int main(int argc, char* argv[]) {
    const char* host = "127.0.0.1";
    int port = 9090;

    if (argc > 1) host = argv[1];
    if (argc > 2) port = std::stoi(argv[2]);

    sockFd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockFd < 0) { perror("socket"); return 1; }

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    inet_pton(AF_INET, host, &addr.sin_addr);

    if (connect(sockFd, (sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("connect");
        return 1;
    }

    std::cout << "╔══════════════════════════════════════╗\n";
    std::cout << "║   TicTacToe Client - Connected!      ║\n";
    std::cout << "║   Commands: MOVE <0-8>  |  QUIT      ║\n";
    std::cout << "╚══════════════════════════════════════╝\n\n";
    std::cout << "Board positions:\n";
    std::cout << "  0 | 1 | 2\n";
    std::cout << " ---+---+---\n";
    std::cout << "  3 | 4 | 5\n";
    std::cout << " ---+---+---\n";
    std::cout << "  6 | 7 | 8\n\n";

    // Start receive thread
    std::thread recvThread(receiveLoop);

    // Send loop
    std::string input;
    while (running && std::getline(std::cin, input)) {
        if (!running) break;
        if (input.empty()) continue;

        // Allow shorthand: just a number
        bool isNumber = true;
        for (char c : input) if (!isdigit(c)) { isNumber = false; break; }
        if (isNumber && input.size() == 1) input = "MOVE " + input;

        // Convert to uppercase command
        std::string cmd;
        size_t spacePos = input.find(' ');
        cmd = (spacePos != std::string::npos) ? input.substr(0, spacePos) : input;
        for (char& c : cmd) c = toupper(c);
        if (spacePos != std::string::npos) cmd += input.substr(spacePos);

        cmd += "\n";
        send(sockFd, cmd.c_str(), cmd.size(), MSG_NOSIGNAL);

        if (cmd.find("QUIT") == 0) {
            running = false;
            break;
        }
    }

    running = false;
    shutdown(sockFd, SHUT_RDWR);
    close(sockFd);
    if (recvThread.joinable()) recvThread.join();

    std::cout << "Goodbye!\n";
    return 0;
}
