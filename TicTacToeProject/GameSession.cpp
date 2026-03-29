#include "GameSession.h"
#include "Protocol.h"

#include <sys/socket.h>
#include <unistd.h>
#include <iostream>
#include <cstring>
#include <errno.h>

GameSession::GameSession(int id) {
    sessionId = id;
    for (int i = 0; i < 9; i++) {
        board[i] = '.'; 
    }
    playerX = -1;
    playerO = -1;
    currentTurn = 'X';
    gameOver = false;
}

bool GameSession::addPlayer(int fd) {
    std::lock_guard<std::mutex> lock(mtx);

    if (playerX == -1) {
        playerX = fd;
        sendToPlayer(fd, "WELCOME X " + std::to_string(sessionId) + "\n");
        sendToPlayer(fd, "WAITING\n");
        return true;
    }

    if (playerO == -1) {
        playerO = fd;
        sendToPlayer(fd, "WELCOME O " + std::to_string(sessionId) + "\n");

        sendToPlayer(playerX, "GAME_START Your turn\n");
        sendToPlayer(playerO, "GAME_START Opponent goes first\n");
        broadcastState();
        return true;
    }

    return false;
}

bool GameSession::handleMove(int fd, int pos) {
    std::lock_guard<std::mutex> lock(mtx);

    if (gameOver) return false;

    char symbol;
    if (fd == playerX) symbol = 'X';
    else if (fd == playerO) symbol = 'O';
    else return false;

    if (symbol != currentTurn) {
        sendToPlayer(fd, "MOVE_ERR Not your turn\n");
        return false;
    }

    if (pos < 0 || pos > 8 || board[pos] != '.') {
        sendToPlayer(fd, "MOVE_ERR Invalid cell\n");
        return false;
    }

    board[pos] = symbol;
    currentTurn = (currentTurn == 'X') ? 'O' : 'X';

    sendToPlayer(fd, "MOVE_OK " + std::to_string(pos) + "\n");

    if (checkWin(symbol)) {
        gameOver = true;
        std::string status = (symbol == 'X') ? "X_WINS" : "O_WINS";
        broadcastStateWithStatus(status);
        std::string resultMsg = "RESULT " + std::string(1, symbol) + " wins\n";
        sendToPlayer(playerX, resultMsg);
        sendToPlayer(playerO, resultMsg);
        return true;
    }

    if (boardFull()) {
        gameOver = true;
        broadcastStateWithStatus("DRAW");
        sendToPlayer(playerX, "RESULT DRAW\n");
        sendToPlayer(playerO, "RESULT DRAW\n");
        return true;
    }

    broadcastState();
    return true;
}


std::string GameSession::buildStateMsg(const std::string& status) const {
    std::string b(board, 9);
    std::string turn(1, currentTurn);
    return Protocol::buildState(b, currentTurn, status);
}

void GameSession::broadcastState() {
    std::string msg = buildStateMsg("IN_PROGRESS");
    if (playerX != -1) sendToPlayer(playerX, msg);
    if (playerO != -1) sendToPlayer(playerO, msg);
}

void GameSession::broadcastStateWithStatus(const std::string& status) {
    std::string msg = buildStateMsg(status);
    if (playerX != -1) sendToPlayer(playerX, msg);
    if (playerO != -1) sendToPlayer(playerO, msg);
}

bool GameSession::checkWin(char s) {
    int w[8][3] = {
        {0,1,2},{3,4,5},{6,7,8},
        {0,3,6},{1,4,7},{2,5,8},
        {0,4,8},{2,4,6}
    };
    for (auto& a : w)
        if (board[a[0]] == s && board[a[1]] == s && board[a[2]] == s)
            return true;
    return false;
}

bool GameSession::boardFull() {
    for (int i = 0; i < 9; i++)
        if (board[i] == '.') return false;
    return true;
}

void GameSession::sendToPlayer(int fd, const std::string& msg) {
    if (fd < 0) return;
    size_t total = 0, len = msg.size();
    while (total < len) {
        ssize_t sent = send(fd, msg.c_str() + total, len - total, MSG_NOSIGNAL);
        if (sent <= 0) { std::cerr << "send error " << strerror(errno) << "\n"; return; }
        total += sent;
    }
}

int GameSession::getOpponent(int fd) const {
    if (fd == playerX) return playerO;
    if (fd == playerO) return playerX;
    return -1;
}
