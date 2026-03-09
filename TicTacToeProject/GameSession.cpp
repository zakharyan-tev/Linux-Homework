#include "GameSession.h"
#include "Protocol.h"
#include <unistd.h>
#include <sys/socket.h>
#include <iostream>
#include <cstring>
#include <errno.h>

bool GameSession::addPlayer(int playerFd) {
    std::lock_guard<std::mutex> lock(stateMutex);

    if (state.playerX_id == -1) {
        state.playerX_id = playerFd;
        sendToPlayer(playerFd, Protocol::buildWelcome('X', sessionId));
        sendToPlayer(playerFd, "WAITING Waiting for opponent...\n");
        return true;
    }

    if (state.playerO_id == -1) {
        state.playerO_id = playerFd;

        sendToPlayer(playerFd, Protocol::buildWelcome('O', sessionId));

        state.status = GameStatus::IN_PROGRESS;

        sendToPlayer(state.playerX_id, "GAME_START It's your turn (X)!\n");
        sendToPlayer(state.playerO_id, "GAME_START Opponent goes first (X)!\n");

        std::string stateMsg = GameLogic::serializeState(state);

        sendToPlayer(state.playerX_id, stateMsg);
        sendToPlayer(state.playerO_id, stateMsg);

        return true;
    }

    return false;
}

bool GameSession::handleMove(int playerFd, int position) {
    std::lock_guard<std::mutex> lock(stateMutex);

    if (!GameLogic::makeMove(state, playerFd, position)) {
        sendToPlayer(playerFd, "MOVE_ERR Invalid move\n");
        return false;
    }

    sendToPlayer(playerFd, "MOVE_OK " + std::to_string(position) + "\n");

    broadcastState();

    return true;
}

void GameSession::broadcastState() const {
    std::string stateMsg = GameLogic::serializeState(state);

    if (state.playerX_id != -1)
        sendToPlayer(state.playerX_id, stateMsg);

    if (state.playerO_id != -1)
        sendToPlayer(state.playerO_id, stateMsg);

    if (state.status != GameStatus::IN_PROGRESS &&
        state.status != GameStatus::WAITING)
    {
        std::string resultMsg;

        switch (state.status) {
            case GameStatus::X_WINS:
                resultMsg = "RESULT X wins! Game over.\n";
                break;
            case GameStatus::O_WINS:
                resultMsg = "RESULT O wins! Game over.\n";
                break;
            case GameStatus::DRAW:
                resultMsg = "RESULT Draw! Game over.\n";
                break;
            default:
                break;
        }

        if (!resultMsg.empty()) {
            if (state.playerX_id != -1)
                sendToPlayer(state.playerX_id, resultMsg);

            if (state.playerO_id != -1)
                sendToPlayer(state.playerO_id, resultMsg);
        }
    }
}

void GameSession::sendToPlayer(int fd, const std::string& msg) {

    if (fd < 0) return;

    size_t total = 0;
    size_t len = msg.size();

    while (total < len) {

        ssize_t sent = send(
            fd,
            msg.c_str() + total,
            len - total,
            MSG_NOSIGNAL
        );

        if (sent <= 0) {
            std::cerr << "[Session] send error fd="
                      << fd << " : "
                      << strerror(errno)
                      << "\n";
            return;
        }

        total += sent;
    }
}

bool GameSession::isFull() const {
    std::lock_guard<std::mutex> lock(stateMutex);
    return state.playerX_id != -1 && state.playerO_id != -1;
}

bool GameSession::isOver() const {
    std::lock_guard<std::mutex> lock(stateMutex);

    return state.status == GameStatus::X_WINS ||
           state.status == GameStatus::O_WINS ||
           state.status == GameStatus::DRAW;
}

int GameSession::getOpponent(int playerFd) const {

    std::lock_guard<std::mutex> lock(stateMutex);

    if (state.playerX_id == playerFd)
        return state.playerO_id;

    if (state.playerO_id == playerFd)
        return state.playerX_id;

    return -1;
}
