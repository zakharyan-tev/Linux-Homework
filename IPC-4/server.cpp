#include <iostream>
#include <string>
#include <list>
#include <pthread.h>
#include <unistd.h>
#include <arpa/inet.h>

#define SERVER_PORT 4004
#define BUFFER_SIZE 1024

struct ClientInfo {
    int fd;
};

std::list<ClientInfo*> clients;
pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER;

void broadcast_message(const std::string& text, int ignore_fd)
{
    pthread_mutex_lock(&clients_mutex);
    for (auto client : clients) {
        if (client->fd != ignore_fd) {
            send(client->fd, text.c_str(), text.length(), 0);
        }
    }
    pthread_mutex_unlock(&clients_mutex);
}

void add_client(ClientInfo* c)
{
    pthread_mutex_lock(&clients_mutex);
    clients.push_back(c);
    pthread_mutex_unlock(&clients_mutex);
}

void remove_client(int fd)
{
    pthread_mutex_lock(&clients_mutex);
    for (auto it = clients.begin(); it != clients.end(); ++it) {
        if ((*it)->fd == fd) {
            delete *it;
            clients.erase(it);
            break;
        }
    }
    pthread_mutex_unlock(&clients_mutex);
}

void* client_thread(void* arg)
{
    ClientInfo* client = static_cast<ClientInfo*>(arg);
    char buffer[BUFFER_SIZE];

    while (true) {
        int r = recv(client->fd, buffer, BUFFER_SIZE - 1, 0);
        if (r <= 0) {
            break;
        }

        buffer[r] = '\0';
        std::string text(buffer);

        if (text.rfind("/exit", 0) == 0) {
            break;
        }

        std::string out = "[msg] " + text;
        broadcast_message(out, client->fd);
    }

    close(client->fd);
    remove_client(client->fd);
    return nullptr;
}

int main()
{
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1) {
        perror("socket");
        return 1;
    }

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(SERVER_PORT);
    addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(server_fd, (sockaddr*)&addr, sizeof(addr)) == -1) {
        perror("bind");
        close(server_fd);
        return 1;
    }

    if (listen(server_fd, 8) == -1) {
        perror("listen");
        close(server_fd);
        return 1;
    }

    std::cout << "Chat server running on port " << SERVER_PORT << std::endl;

    while (true) {
        sockaddr_in client_addr{};
        socklen_t addr_len = sizeof(client_addr);

        int client_fd = accept(server_fd, (sockaddr*)&client_addr, &addr_len);
        if (client_fd < 0) {
            continue;
        }

        ClientInfo* client = new ClientInfo{client_fd};
        add_client(client);

        pthread_t th;
        pthread_create(&th, nullptr, client_thread, client);
        pthread_detach(th);
    }

    close(server_fd);
    return 0;
}
