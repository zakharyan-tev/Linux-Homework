#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define MAX_CLIENTS 10
#define BUFFER_SIZE 2048

typedef struct {
    struct sockaddr_in address;
    int sockfd;
    int uid;
    char name[32];
} client_t;

client_t *clients[MAX_CLIENTS];
pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER;

void add_client(client_t *cl) {
    pthread_mutex_lock(&clients_mutex);
    for (int i = 0; i < MAX_CLIENTS; ++i) {
        if (!clients[i]) {
            clients[i] = cl;
            break;
        }
    }
    pthread_mutex_unlock(&clients_mutex);
}

void remove_client(int uid) {
    pthread_mutex_lock(&clients_mutex);
    for (int i = 0; i < MAX_CLIENTS; ++i) {
        if (clients[i] && clients[i]->uid == uid) {
            clients[i] = NULL;
            break;
        }
    }
    pthread_mutex_unlock(&clients_mutex);
}

void send_message(char *s, int uid) {
    pthread_mutex_lock(&clients_mutex);
    for (int i = 0; i < MAX_CLIENTS; ++i) {
        if (clients[i] && clients[i]->uid != uid) {
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
        printf("Didn't enter the name.\n");
        leave_flag = 1;
    } else {
        strcpy(cli->name, name);
        sprintf(buff_out, "%s has joined\n", cli->name);
        printf("%s", buff_out);
        send_message(buff_out, cli->uid);
    }

    bzero(buff_out, BUFFER_SIZE);

    while (1) {
        if (leave_flag) break;

        int receive = recv(cli->sockfd, buff_out, BUFFER_SIZE, 0);
        if (receive > 0) {
            if (strlen(buff_out) > 0) {
                if (strcmp(buff_out, "/exit") == 0) {
                    sprintf(buff_out, "%s has left\n", cli->name);
                    printf("%s", buff_out);
                    send_message(buff_out, cli->uid);
                    leave_flag = 1;
                } 

                else if (strcmp(buff_out, "/list") == 0) {
                    char list[BUFFER_SIZE] = "Online users: ";
                    pthread_mutex_lock(&clients_mutex);
                    for(int i=0; i<MAX_CLIENTS; i++) {
                        if(clients[i]) {
                            strcat(list, clients[i]->name);
                            strcat(list, " ");
                        }
                    }
                    pthread_mutex_unlock(&clients_mutex);
                    strcat(list, "\n");
                    send(cli->sockfd, list, strlen(list), 0);
                }
                    
                else {
                    char send_buffer[BUFFER_SIZE + 50];
                    sprintf(send_buffer, "%s: %s", cli->name, buff_out);
                    send_message(send_buffer, cli->uid);
                }
            }
        } else if (receive == 0 || strcmp(buff_out, "/exit") == 0) {
            sprintf(buff_out, "%s has left\n", cli->name);
            printf("%s", buff_out);
            send_message(buff_out, cli->uid);
            leave_flag = 1;
        } else {
            perror("ERROR: -1");
            leave_flag = 1;
        }
        bzero(buff_out, BUFFER_SIZE);
    }

    close(cli->sockfd);
    remove_client(cli->uid);
    free(cli);
    pthread_detach(pthread_self());

    return NULL;
}

int main() {
    int listenfd = 0, connfd = 0;
    struct sockaddr_in serv_addr;
    pthread_t tid;

    listenfd = socket(AF_INET, SOCK_STREAM, 0);
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(8888);

    if (bind(listenfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("ERROR: Socket binding failed");
        return 1;
    }

    if (listen(listenfd, 10) < 0) {
        perror("ERROR: Socket listening failed");
        return 1;
    }

    printf("--- WELCOME TO THE CHATROOM ---\n");

    while (1) {
        struct sockaddr_in cli_addr;
        socklen_t clilen = sizeof(cli_addr);
        connfd = accept(listenfd, (struct sockaddr*)&cli_addr, &clilen);

        client_t *cli = (client_t *)malloc(sizeof(client_t));
        cli->address = cli_addr;
        cli->sockfd = connfd;
        cli->uid = connfd;

        add_client(cli);
        pthread_create(&tid, NULL, &handle_client, (void*)cli);
        sleep(1);
    }

    return 0;
}
