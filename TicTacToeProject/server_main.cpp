#include "Server.h"
#include <iostream>
#include <csignal>
#include <stdexcept>

static Server* g_server = nullptr;

void signalHandler(int sig) {
    std::cout << "\n[Server] Signal " << sig << " received. Shutting down...\n";
    if (g_server) g_server->stop();
    exit(0);
}

int main(int argc, char* argv[]) {
    int port = 9090;
    if (argc > 1) {
        try { port = std::stoi(argv[1]); }
        catch (...) {
            std::cerr << "Invalid port. Using default 9090\n";
        }
    }

    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);
    signal(SIGPIPE, SIG_IGN); // Ignore broken pipe

    try {
        Server server(port);
        g_server = &server;
        server.start();
    } catch (const std::exception& e) {
        std::cerr << "[Server] Fatal error: " << e.what() << "\n";
        return 1;
    }

    return 0;
}
