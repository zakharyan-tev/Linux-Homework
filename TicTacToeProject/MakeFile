CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -O2
LDFLAGS = -pthread

SERVER_SRCS = server_main.cpp \
              Server.cpp \
              GameSession.cpp \
              GameLogic.cpp \
              Protocol.cpp

CLIENT_SRCS = client_main.cpp

SERVER_BIN = tictactoe_server
CLIENT_BIN = tictactoe_client

all: server client

server:
	$(CXX) $(CXXFLAGS) -o $(SERVER_BIN) $(SERVER_SRCS) $(LDFLAGS)

client:
	$(CXX) $(CXXFLAGS) -o $(CLIENT_BIN) $(CLIENT_SRCS) $(LDFLAGS)

clean:
	rm -f $(SERVER_BIN) $(CLIENT_BIN)
