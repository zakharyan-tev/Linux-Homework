#pragma once

#include <string>
#include <mutex>

class GameSession
{
public:
    int sessionId;

    GameSession(int id);

    bool addPlayer(int fd);
    bool handleMove(int fd, int pos);
    int  getOpponent(int fd) const;

    static void sendToPlayer(int fd, const std::string& msg);

private:
    void broadcastState();
    void broadcastStateWithStatus(const std::string& status);
    std::string buildStateMsg(const std::string& status) const;

    bool checkWin(char symbol);
    bool boardFull();

private:
    int  playerX;
    int  playerO;
    char board[9];
    char currentTurn;
    bool gameOver;

    mutable std::mutex mtx;
};
