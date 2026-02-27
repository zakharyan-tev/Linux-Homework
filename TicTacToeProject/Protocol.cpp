#include "Protocol.h"
#include <sstream>

namespace Protocol {

std::string buildWelcome(char symbol, int sessionId) {
    return "WELCOME " + std::string(1, symbol) + " " + std::to_string(sessionId) + "\n";
}

std::string buildState(const std::string& board, char turn, const std::string& status) {
    return "STATE " + board + " " + turn + " " + status + "\n";
}

std::string buildBoard(const std::string& board) {
    // board is 9 chars
    std::ostringstream oss;
    oss << "\n";
    oss << "  0 | 1 | 2\n";
    oss << " ---+---+---\n";
    oss << "  " << board[0] << " | " << board[1] << " | " << board[2] << "   (0-2)\n";
    oss << " ---+---+---\n";
    oss << "  " << board[3] << " | " << board[4] << " | " << board[5] << "   (3-5)\n";
    oss << " ---+---+---\n";
    oss << "  " << board[6] << " | " << board[7] << " | " << board[8] << "   (6-8)\n";
    oss << "\n";
    return "BOARD " + oss.str();
}

std::string parseCommand(const std::string& msg) {
    auto pos = msg.find(' ');
    if (pos == std::string::npos) return msg;
    return msg.substr(0, pos);
}

int parseMovePosition(const std::string& msg) {
    auto pos = msg.find(' ');
    if (pos == std::string::npos) return -1;
    try {
        return std::stoi(msg.substr(pos + 1));
    } catch (...) {
        return -1;
    }
}

} // namespace Protocol
