#include <iostream>
#include <thread>
#include <string>
#include <vector>
#include <atomic>
#include <unistd.h>
#include <arpa/inet.h>

std::string encode(const std::vector<std::string>& parts) {
    std::string out;
    for (const auto& s : parts) {
        out += std::to_string(s.size()) + "#" + s;
    }
    return out;
}

std::vector<std::string> decode(const std::string& raw, size_t& consumed) {
    std::vector<std::string> result;
    size_t pos = 0;
    while (pos < raw.size()) {
        size_t hash = raw.find('#', pos);
        if (hash == std::string::npos) break;
        size_t len = std::stoul(raw.substr(pos, hash - pos));
        size_t start = hash + 1;
        if (start + len > raw.size()) break;
        result.push_back(raw.substr(start, len));
        pos = start + len;
    }
    consumed = pos;
    return result;
}

std::atomic<bool> running{true};

void receive_loop(int sock) {
    char buf[4096];
    std::string accum;

    while (running) {
        int n = recv(sock, buf, sizeof(buf), 0);
        if (n <= 0) {
            std::cout << "\n[Disconnected from server]\n";
            running = false;
            break;
        }
        accum += std::string(buf, n);
        size_t consumed = 0;
        auto msgs = decode(accum, consumed);
        accum = accum.substr(consumed);
        for (const auto& m : msgs) {
            std::cout << m << "\n";
        }
    }
}

void print_help() {
    std::cout <<
        "\n--- Local help ---\n"
        "  /rooms                       list all rooms\n"
        "  /who                         list online users\n"
        "  /create <name>               create public room\n"
        "  /create <name> private <code>  create private room\n"
        "  /join <name>                 join public room\n"
        "  /join <name> <code>          join private room\n"
        "  /leave                       leave current room\n"
        "  /pm <name> <message>         private message\n"
        "  /help                        this help\n"
        "  /quit                        disconnect\n"
        "------------------\n";
}

int main() {
    int sock = socket(AF_INET, SOCK_STREAM, 0);

    sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port   = htons(12345);
    inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr);

    if (connect(sock, (sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        std::cerr << "Cannot connect to server.\n";
        return 1;
    }

    std::string name;
    std::cout << "Enter your name: ";
    std::getline(std::cin, name);
    if (name.empty()) name = "Anonymous";

    std::string frame = encode({name});
    send(sock, frame.c_str(), frame.size(), 0);

    print_help();

    std::thread(receive_loop, sock).detach();

    std::string line;
    while (running && std::getline(std::cin, line)) {
        if (!running) break;

        if (line == "/quit") {
            break;
        }
        if (line == "/help") {
            print_help();
            continue;
        }

        std::string out = encode({line});
        if (send(sock, out.c_str(), out.size(), 0) <= 0) {
            std::cout << "[Send failed. Server may have closed.]\n";
            break;
        }
    }

    running = false;
    close(sock);
    std::cout << "Bye.\n";
    return 0;
}
