CC = gcc
CFLAGS = -Wall -pthread
SRC_DIR = src
BIN_DIR = .

all: server client

server: $(SRC_DIR)/server.c
	$(CC) $(CFLAGS) $(SRC_DIR)/server.c -o $(BIN_DIR)/server

client: $(SRC_DIR)/client.c
	$(CC) $(CFLAGS) $(SRC_DIR)/client.c -o $(BIN_DIR)/client

clean:
	rm -f server client
