#include "TcpServer.hpp"
#include <iostream>

int main()
{
    SimpleNet::TcpServer server(1111, 4);

    std::cout << "Server started on port 1111\n";

    server.run([](SimpleNet::Socket client) {
        auto data = client.receive();
        client.send("Hello client\n");
    });

    return 0;
}
