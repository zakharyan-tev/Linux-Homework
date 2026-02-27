#include "GameLogic.h"
#include "Protocol.h"
#include <sstream>

bool GameLogic::makeMove(GameState& state, int playerFd, int position) {
    if (state.status != GameStatus::IN_PROGRESS) return false;
    if (position < 0 || position > 8) return false;
    if (state.board[position] != Cell::EMPTY) return false;

    // Check it's the right player's turn
    bool isX = (state.currentTurn == Cell::X && state.playerX_id == playerFd);
    bool isO = (state.currentTurn == Cell::O && state.playerO_id == playerFd);
    if (!isX && !isO) return false;

    state.board[position] = state.currentTurn;
    state.moveCount++;

    state.status = checkWinner(state);
    if (state.status == GameStatus::IN_PROGRESS) {
        state.currentTurn = (state.currentTurn == Cell::X) ? Cell::O : Cell::X;
    }
    return true;
}

GameStatus GameLogic::checkWinner(const GameState& state) {
    const auto& b = state.board;
    // All winning combinations
    const int wins[8][3] = {
        {0,1,2},{3,4,5},{6,7,8}, // rows
        {0,3,6},{1,4,7},{2,5,8}, // cols
        {0,4,8},{2,4,6}          // diags
    };
    for (auto& w : wins) {
        if (b[w[0]] != Cell::EMPTY && b[w[0]] == b[w[1]] && b[w[1]] == b[w[2]]) {
            return (b[w[0]] == Cell::X) ? GameStatus::X_WINS : GameStatus::O_WINS;
        }
    }
    if (isBoardFull(state)) return GameStatus::DRAW;
    return GameStatus::IN_PROGRESS;
}

bool GameLogic::isBoardFull(const GameState& state) {
    for (auto& c : state.board)
        if (c == Cell::EMPTY) return false;
    return true;
}

std::string GameLogic::cellToChar(Cell c) {
    switch(c) {
        case Cell::X: return "X";
        case Cell::O: return "O";
        default: return ".";
    }
}

std::string GameLogic::statusToString(GameStatus s) {
    switch(s) {
        case GameStatus::WAITING:     return "WAITING";
        case GameStatus::IN_PROGRESS: return "IN_PROGRESS";
        case GameStatus::X_WINS:      return "X_WINS";
        case GameStatus::O_WINS:      return "O_WINS";
        case GameStatus::DRAW:        return "DRAW";
    }
    return "UNKNOWN";
}

std::string GameLogic::serializeState(const GameState& state) {
    std::string board;
    for (auto& c : state.board)
        board += cellToChar(c);
    std::string turn = cellToChar(state.currentTurn);
    std::string status = statusToString(state.status);
    return Protocol::buildState(board, turn[0], status);
}
