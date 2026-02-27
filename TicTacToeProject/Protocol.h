#pragma once
#include <string>

// Protocol: All messages are newline-terminated strings
// Client -> Server:
//   MOVE <position>       (position 0-8)
//   QUIT
//
// Server -> Client:
//   WELCOME <symbol> <session_id>
//   STATE <board> <turn> <status>   (board = 9 chars: X/O/.)
//   MOVE_OK <position>
//   MOVE_ERR <reason>
//   WAITING
//   OPPONENT_DISCONNECTED
//   BOARD                 (pretty-printed board follows)

namespace Protocol {
    const std::string WELCOME    = "WELCOME";
    const std::string STATE      = "STATE";
    const std::string MOVE_CMD   = "MOVE";
    const std::string MOVE_OK    = "MOVE_OK";
    const std::string MOVE_ERR   = "MOVE_ERR";
    const std::string WAITING    = "WAITING";
    const std::string QUIT_CMD   = "QUIT";
    const std::string DISCONNECTED = "OPPONENT_DISCONNECTED";

    std::string buildWelcome(char symbol, int sessionId);
    std::string buildState(const std::string& board, char turn, const std::string& status);
    std::string buildBoard(const std::string& board);
    std::string parseCommand(const std::string& msg);
    int parseMovePosition(const std::string& msg);
}
