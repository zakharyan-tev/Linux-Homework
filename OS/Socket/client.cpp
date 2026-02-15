#include "Socket.hpp"
#include <iostream>
#include <string>

int main() 
{
    try {
        SimpleNet::Socket client;

        client.connect("127.0.0.1", 1111);
        client.send("Hello server\n");

        auto response = client.receive();
        std::string msg(response.begin(), response.end());

        std::cout << "Server responded: " << msg << std::endl;
    }
    catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }

    return 0;
}
