#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include "chat.h"

#define MAX_CLIENTS 10
#define BUFFER_SIZE 2048

typedef struct {
    int sockfd;
    char name[32];
} client_t;

client_t *clients[MAX_CLIENTS];
pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER;

void send_message(char *s, int uid) {
    pthread_mutex_lock(&clients_mutex);
    for (int i = 0; i < MAX_CLIENTS; ++i) {
        if (clients[i] && clients[i]->sockfd != uid) {
            if (write(clients[i]->sockfd, s, strlen(s)) < 0) {
                perror("ERROR: write to descriptor failed");
                break;
            }
        }
    }
    pthread_mutex_unlock(&clients_mutex);
}

void *handle_client(void *arg) {
    char buff_out[BUFFER_SIZE];
    char name[32];
    int leave_flag = 0;
    client_t *cli = (client_t *)arg;

    if (recv(cli->sockfd, name, 32, 0) <= 0 || strlen(name) < 2 || strlen(name) >= 31) {
        printf("Error: Invalid name\n");
        leave_flag = 1;
    } else {
        strcpy(cli->name, name);
        sprintf(buff_out, ">>> %s has joined the chat\n", cli->name);
        printf("%s", buff_out);
        send_message(buff_out, cli->sockfd);
    }

    bzero(buff_out, BUFFER_SIZE);

    while (1) {
        if (leave_flag) break;

        int receive = recv(cli->sockfd, buff_out, BUFFER_SIZE, 0);
        if (receive > 0) {
            if (strlen(buff_out) > 0) {
                if (strcmp(buff_out, "/list") == 0) {
                    char list[BUFFER_SIZE] = "Online users: ";
                    pthread_mutex_lock(&clients_mutex);
                    for (int i = 0; i < MAX_CLIENTS; i++) {
                        if (clients[i]) {
                            strcat(list, clients[i]->name);
                            strcat(list, " ");
                        }
                    }
                    pthread_mutex_unlock(&clients_mutex);
                    strcat(list, "\n");
                    write(cli->sockfd, list, strlen(list));
                } 
                else {
                    char final_msg[BUFFER_SIZE + 50];
                    sprintf(final_msg, "[%s]: %s\n", cli->name, buff_out);
                    send_message(final_msg, cli->sockfd);
                }
            }
        } else if (receive == 0 || strcmp(buff_out, "/exit") == 0) {
            sprintf(buff_out, "<<< %s has left\n", cli->name);
            printf("%s", buff_out);
            send_message(buff_out, cli->sockfd);
            leave_flag = 1;
        } else {
            printf("ERROR: -1\n");
            leave_flag = 1;
        }
        bzero(buff_out, BUFFER_SIZE);
    }

    close(cli->sockfd);
    pthread_mutex_lock(&clients_mutex);
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i] && clients[i]->sockfd == cli->sockfd) {
            clients[i] = NULL;
            break;
        }
    }
    pthread_mutex_unlock(&clients_mutex);
    free(cli);
    pthread_detach(pthread_self());
    return NULL;
}

int main(int argc, char **argv) {
    int listenfd, connfd;
    struct sockaddr_in serv_addr, cli_addr;
    pthread_t tid;

    listenfd = socket(AF_INET, SOCK_STREAM, 0);
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(8888);

    bind(listenfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr));
    listen(listenfd, 10);

    printf("--- SERVER STARTED ---\n");

    while (1) {
        socklen_t clilen = sizeof(cli_addr);
        connfd = accept(listenfd, (struct sockaddr*)&cli_addr, &clilen);

        pthread_mutex_lock(&clients_mutex);
        client_t *cli = (client_t *)malloc(sizeof(client_t));
        cli->sockfd = connfd;
        
        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (!clients[i]) {
                clients[i] = cli;
                pthread_create(&tid, NULL, &handle_client, (void*)cli);
                break;
            }
        }
        pthread_mutex_unlock(&clients_mutex);
    }
    return 0;
}
