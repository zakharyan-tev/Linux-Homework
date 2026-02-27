#pragma once
#include "GameSession.h"
#include <unordered_map>
#include <vector>
#include <memory>
#include <mutex>
#include <atomic>
#include <thread>

class Server {
public:
    explicit Server(int port);
    ~Server();

    void start();
    void stop();

private:
    int port_;
    int serverFd_;
    std::atomic<bool> running_{false};
    std::atomic<int> nextSessionId_{1};

    // fd -> sessionId mapping
    std::unordered_map<int, int> clientToSession_;
    std::mutex clientMapMutex_;

    // sessionId -> GameSession
    std::unordered_map<int, std::shared_ptr<GameSession>> sessions_;
    std::mutex sessionsMutex_;

    // Pending session waiting for second player
    std::shared_ptr<GameSession> pendingSession_;
    std::mutex pendingMutex_;

    void acceptLoop();
    void handleClient(int clientFd);
    void processMessage(int clientFd, const std::string& msg);
    void cleanupClient(int clientFd);
    std::shared_ptr<GameSession> getOrCreateSession(int clientFd);

    static void setSocketOptions(int fd);
};
