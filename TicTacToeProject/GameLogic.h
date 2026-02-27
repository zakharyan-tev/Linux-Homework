#pragma once
#include <array>
#include <string>
#include <optional>

enum class Cell { EMPTY, X, O };
enum class GameStatus { WAITING, IN_PROGRESS, X_WINS, O_WINS, DRAW };

struct GameState {
    std::array<Cell, 9> board;
    Cell currentTurn;
    GameStatus status;
    int playerX_id;
    int playerO_id;
    int moveCount;

    GameState() : board{}, currentTurn(Cell::X), status(GameStatus::WAITING),
                  playerX_id(-1), playerO_id(-1), moveCount(0) {
        board.fill(Cell::EMPTY);
    }
};

class GameLogic {
public:
    static bool makeMove(GameState& state, int playerFd, int position);
    static GameStatus checkWinner(const GameState& state);
    static std::string serializeState(const GameState& state);
    static std::string cellToChar(Cell c);
    static bool isBoardFull(const GameState& state);
    static std::string statusToString(GameStatus s);
};
