#pragma once
#include "GameLogic.h"
#include <mutex>
#include <atomic>
#include <string>

class GameSession {
public:
    int sessionId;
    GameState state;
    mutable std::mutex stateMutex;
    std::atomic<bool> active{true};

    explicit GameSession(int id) : sessionId(id) {}

    // Returns true if player successfully joined
    bool addPlayer(int playerFd);

    // Returns true if move was valid and applied
    bool handleMove(int playerFd, int position);

    // Broadcast current state to both players
    void broadcastState() const;

    // Send message to a specific player
    static void sendToPlayer(int fd, const std::string& msg);

    bool isFull() const;
    bool isOver() const;
    int getOpponent(int playerFd) const;
};
