#include "TcpServer.hpp"
#include <thread>

namespace SimpleNet {

TcpServer::TcpServer(int port, size_t threads)
    : pool_(threads)
{
    listen_socket_.bind(port);
    listen_socket_.listen();
}


void TcpServer::run(ClientHandler handler) {
    while (true) {
        Socket client = listen_socket_.accept();

        pool_.enqueue(
            [handler, client = std::move(client)]() mutable {
                handler(std::move(client));
            }
        );
    }
}

}
