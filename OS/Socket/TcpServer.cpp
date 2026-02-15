#include "TcpServer.hpp"
#include <thread>

namespace SimpleNet {

TcpServer::TcpServer(int port) {
    listen_socket_.bind(port);
    listen_socket_.listen();
}

void TcpServer::run(ClientHandler handler) {
    while (true) {
        Socket client = listen_socket_.accept();
        std::thread([handler, client = std::move(client)]() mutable {
            handler(std::move(client));
        }).detach();
    }
}

}
