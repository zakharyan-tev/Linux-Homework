#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "chat.h"

#define BUFFER_SIZE 2048

int sockfd = 0;
char name[32];

void *recv_msg_handler(void *arg) {
    char message[BUFFER_SIZE];
    while (1) {
        int receive = recv(sockfd, message, BUFFER_SIZE, 0);
        if (receive > 0) {
            printf("%s", message);
        } else if (receive == 0) {
            break;
        }
        memset(message, 0, BUFFER_SIZE);
    }
    return NULL;
}

int main() {
    struct sockaddr_in serv_addr;

    printf("Enter your name: ");
    fgets(name, 32, stdin);
    name[strcspn(name, "\n")] = 0;

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    serv_addr.sin_port = htons(8888);

    if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) == -1) {
        printf("ERROR: connect failed\n");
        return 1;
    }

    send(sockfd, name, 32, 0);

    pthread_t recv_thread;
    pthread_create(&recv_thread, NULL, recv_msg_handler, NULL);

    char buffer[BUFFER_SIZE];
    while (1) {
        fgets(buffer, BUFFER_SIZE, stdin);
        buffer[strcspn(buffer, "\n")] = 0;

        if (strcmp(buffer, "/exit") == 0) {
            break;
        } else {
            send(sockfd, buffer, strlen(buffer), 0);
        }
        memset(buffer, 0, BUFFER_SIZE);
    }

    close(sockfd);
    return 0;
}
