#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define BUFFER_SIZE 2048

int sockfd = 0;
char name[32];

void str_trim_lf (char* arr, int length) {
  for (int i = 0; i < length; i++) {
    if (arr[i] == '\n') {
      arr[i] = '\0';
      break;
    }
  }
}

void *recv_msg_handler() {
	char message[BUFFER_SIZE];
  while (1) {
		int receive = recv(sockfd, message, BUFFER_SIZE, 0);
    if (receive > 0) {
      printf("%s", message);
    } else if (receive == 0) {
			break;
    }
		memset(message, 0, sizeof(message));
  }
  return NULL;
}

int main() {
	struct sockaddr_in serv_addr;

	printf("Enter your name: ");
	fgets(name, 32, stdin);
	str_trim_lf(name, strlen(name));

	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
	serv_addr.sin_port = htons(8888);

	if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) == -1) {
		printf("ERROR: connect\n");
		return 1;
	}

	send(sockfd, name, 32, 0);

	pthread_t recv_msg_thread;
  if(pthread_create(&recv_msg_thread, NULL, (void *) recv_msg_handler, NULL) != 0){
		printf("ERROR: pthread\n");
		return 1;
	}

	while (1) {
		char buffer[BUFFER_SIZE];
		fgets(buffer, BUFFER_SIZE, stdin);
		str_trim_lf(buffer, BUFFER_SIZE);

		if (strcmp(buffer, "/exit") == 0) {
            send(sockfd, buffer, strlen(buffer), 0);
			break;
		} else {
			send(sockfd, buffer, strlen(buffer), 0);
		}
		bzero(buffer, BUFFER_SIZE);
	}

	close(sockfd);
	return 0;
}
