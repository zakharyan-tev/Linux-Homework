#include <iostream>
#include <string>
#include <pthread.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 4004
#define BUFFER_LEN 1024

int client_fd;

void* receive_loop(void* arg)
{
    char buffer[BUFFER_LEN];

    while (true) {
        int received = recv(client_fd, buffer, BUFFER_LEN - 1, 0);
        if (received <= 0) {
            break;
        }

        buffer[received] = '\0';
        std::cout << buffer << std::flush;
    }

    _exit(0);
    return nullptr;
}

int main()
{
    sockaddr_in server_addr{};
    client_fd = socket(AF_INET, SOCK_STREAM, 0);

    if (client_fd == -1) {
        perror("socket");
        return 1;
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    if (connect(client_fd, (sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        perror("connect");
        close(client_fd);
        return 1;
    }

    pthread_t recv_thread;
    pthread_create(&recv_thread, nullptr, receive_loop, nullptr);

    std::string input;
    while (true) {
        std::getline(std::cin, input);

        if (input.rfind("/exit", 0) == 0) {
            break;
        }

        input.push_back('\n');
        send(client_fd, input.c_str(), input.length(), 0);
    }

    close(client_fd);
    return 0;
}


