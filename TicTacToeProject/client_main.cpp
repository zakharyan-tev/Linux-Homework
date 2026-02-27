#include <iostream>
#include <string>
#include <thread>
#include <atomic>
#include <mutex>
#include <sstream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>

// ─── ANSI colours ─────────────────────────────────────────────────────────────
#define RESET   "\033[0m"
#define BOLD    "\033[1m"
#define DIM     "\033[2m"
#define RED     "\033[31m"
#define GREEN   "\033[32m"
#define YELLOW  "\033[33m"
#define CYAN    "\033[36m"
#define WHITE   "\033[97m"

// ─── Shared state ─────────────────────────────────────────────────────────────
std::atomic<bool> running{true};
int               sockFd = -1;
std::mutex        displayMutex;

char        mySymbol    = '?';
int         sessionId   = -1;
std::string board       = ".........";  // 9 cells
char        currentTurn = 'X';
std::string gameStatus  = "WAITING";
int         moveCount   = 0;
std::string lastMessage;

// ─── Helpers ─────────────────────────────────────────────────────────────────
static void clearScreen() {
    std::cout << "\033[2J\033[H";
}

// ─── Full UI redraw ───────────────────────────────────────────────────────────
static void redrawUI() {
    clearScreen();

    char oppSymbol = (mySymbol == 'X') ? 'O' : 'X';
    bool myTurn    = (currentTurn == mySymbol) && (gameStatus == "IN_PROGRESS");
    bool gameOver  = (gameStatus == "X_WINS" || gameStatus == "O_WINS" || gameStatus == "DRAW");

    // ── Header ────────────────────────────────────────────────────────────────
    std::cout << BOLD << WHITE
              << "╔══════════════════════════════════════════╗\n"
              << "║       🎮  TicTacToe  Multiplayer         ║\n"
              << "╚══════════════════════════════════════════╝\n"
              << RESET << "\n";

    // ── Player badges ─────────────────────────────────────────────────────────
    //   [ You: X ]   vs   [ Opp: O ]   with active turn highlighted
    auto badge = [&](char sym, const std::string& label, bool active) {
        if (active)
            std::cout << BOLD << GREEN << "▶ " << label << " [" << sym << "] ◀" << RESET;
        else
            std::cout << DIM  << WHITE << "  " << label << " [" << sym << "]  " << RESET;
    };

    std::cout << "  ";
    badge(mySymbol,  "You ", myTurn);
    std::cout << "       ";
    badge(oppSymbol, "Opp ", !myTurn && !gameOver);
    std::cout << "\n\n";

    // ── Scoreboard / move stats ────────────────────────────────────────────────
    // Count each player's pieces
    int myPieces  = 0, oppPieces = 0;
    for (char c : board) {
        if (c == mySymbol)  ++myPieces;
        if (c == oppSymbol) ++oppPieces;
    }
    std::cout << DIM << WHITE
              << "  Your pieces: " << myPieces
              << "   |   Opponent pieces: " << oppPieces
              << "   |   Total moves: " << moveCount
              << "\n" << RESET << "\n";

    // ── Board ─────────────────────────────────────────────────────────────────
    auto cellStr = [&](int idx) -> std::string {
        char c = board[idx];
        if (c == mySymbol)
            return std::string(BOLD) + GREEN + c + RESET;
        if (c == oppSymbol)
            return std::string(BOLD) + RED   + c + RESET;
        // Empty: show position number dimmed
        return std::string(DIM) + WHITE + (char)('0' + idx) + RESET;
    };

    std::cout << "     0   1   2\n";
    std::cout << "  ┌───┬───┬───┐\n";
    for (int row = 0; row < 3; ++row) {
        std::cout << "  │ " << cellStr(row*3)   << " │ "
                            << cellStr(row*3+1) << " │ "
                            << cellStr(row*3+2) << " │  row " << (row+1) << "\n";
        if (row < 2)
            std::cout << "  ├───┼───┼───┤\n";
    }
    std::cout << "  └───┴───┴───┘\n";

    // ── Status ───────────────────────────────────────────────────────────────
    std::cout << "\n";
    if (gameStatus == "WAITING") {
        std::cout << YELLOW << BOLD << "  ⏳ Waiting for opponent...\n" << RESET;
    } else if (gameStatus == "IN_PROGRESS") {
        if (myTurn)
            std::cout << GREEN << BOLD << "  ➤ Your turn!  Type: MOVE <0-8>  or just a digit\n" << RESET;
        else
            std::cout << RED   << BOLD << "  ⌛ Opponent is thinking...\n" << RESET;
    } else if (gameStatus == "X_WINS" || gameStatus == "O_WINS") {
        char winner = (gameStatus == "X_WINS") ? 'X' : 'O';
        if (winner == mySymbol)
            std::cout << GREEN << BOLD << "  🏆 You win! Congratulations!\n" << RESET;
        else
            std::cout << RED   << BOLD << "  💀 You lose. Better luck next time!\n" << RESET;
    } else if (gameStatus == "DRAW") {
        std::cout << YELLOW << BOLD << "  🤝 It's a draw!\n" << RESET;
    }

    std::cout << DIM << "  Session #" << sessionId << "\n" << RESET;

    // ── Last hint / error ────────────────────────────────────────────────────
    if (!lastMessage.empty()) {
        std::cout << "\n  " << YELLOW << lastMessage << RESET << "\n";
    }

    std::cout << "\n  > " << std::flush;
}

// ─── Receive thread ───────────────────────────────────────────────────────────
void receiveLoop() {
    char buf[2048];
    std::string buffer;

    while (running) {
        ssize_t n = recv(sockFd, buf, sizeof(buf) - 1, 0);
        if (n <= 0) {
            if (running) {
                std::lock_guard<std::mutex> lk(displayMutex);
                lastMessage = "⚠️  Disconnected from server.";
                redrawUI();
            }
            running = false;
            break;
        }
        buf[n] = '\0';
        buffer += buf;

        size_t pos;
        while ((pos = buffer.find('\n')) != std::string::npos) {
            std::string line = buffer.substr(0, pos);
            buffer.erase(0, pos + 1);
            if (!line.empty() && line.back() == '\r') line.pop_back();
            if (line.empty()) continue;

            std::lock_guard<std::mutex> lk(displayMutex);

            if (line.substr(0, 7) == "WELCOME") {
                // WELCOME X 1
                std::istringstream ss(line.substr(8));
                std::string sym; int sid;
                ss >> sym >> sid;
                mySymbol  = sym.empty() ? '?' : sym[0];
                sessionId = sid;
                lastMessage = "Connected as [" + sym + "]  — waiting for opponent";
                redrawUI();

            } else if (line.substr(0, 5) == "STATE") {
                // STATE <board9> <turn> <status>
                std::istringstream ss(line.substr(6));
                std::string b, turn, status;
                ss >> b >> turn >> status;
                if (b.size() == 9)   board = b;
                if (!turn.empty())   currentTurn = turn[0];
                if (!status.empty()) gameStatus  = status;
                moveCount = 0;
                for (char c : board) if (c != '.') ++moveCount;
                // Keep error messages; clear neutral ones
                if (lastMessage.find("❌") == std::string::npos)
                    lastMessage.clear();
                redrawUI();

            } else if (line.substr(0, 7) == "MOVE_OK") {
                lastMessage = "✔ Move placed at position " + line.substr(8);
                redrawUI();

            } else if (line.substr(0, 8) == "MOVE_ERR") {
                lastMessage = "❌ " + line.substr(9);
                redrawUI();

            } else if (line.substr(0, 10) == "GAME_START") {
                lastMessage = "🎯 " + line.substr(11);
                redrawUI();

            } else if (line.substr(0, 6) == "RESULT") {
                lastMessage = "🏆 " + line.substr(7);
                redrawUI();
                running = false;

            } else if (line == "OPPONENT_DISCONNECTED") {
                gameStatus  = "DRAW";
                lastMessage = "⚠️  Opponent disconnected!";
                redrawUI();
                running = false;

            } else if (line.substr(0, 5) == "BOARD") {
                // Server's text board — we draw our own, skip

            } else if (line.substr(0, 4) == "TURN") {
                // Redundant with STATE, skip

            } else if (line.substr(0, 7) == "WAITING") {
                gameStatus  = "WAITING";
                lastMessage = "";
                redrawUI();

            } else {
                // INFO or other
                lastMessage = line;
                redrawUI();
            }
        }
    }
}

// ─── Main ─────────────────────────────────────────────────────────────────────
int main(int argc, char* argv[]) {
    const char* host = "127.0.0.1";
    int port = 9090;
    if (argc > 1) host = argv[1];
    if (argc > 2) port = std::stoi(argv[2]);

    sockFd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockFd < 0) { perror("socket"); return 1; }

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port   = htons(port);
    inet_pton(AF_INET, host, &addr.sin_addr);

    if (connect(sockFd, (sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("connect");
        return 1;
    }

    {
        std::lock_guard<std::mutex> lk(displayMutex);
        redrawUI();
    }

    std::thread recvThread(receiveLoop);

    std::string input;
    while (running && std::getline(std::cin, input)) {
        if (!running) break;

        if (input.empty()) {
            // Refresh on blank enter
            std::lock_guard<std::mutex> lk(displayMutex);
            redrawUI();
            continue;
        }

        // Single digit shorthand → MOVE N
        if (input.size() == 1 && isdigit(input[0]))
            input = "MOVE " + input;

        // Uppercase command word
        size_t sp = input.find(' ');
        std::string head = (sp != std::string::npos) ? input.substr(0, sp) : input;
        for (char& c : head) c = toupper(c);
        std::string cmd = head + (sp != std::string::npos ? input.substr(sp) : "") + "\n";

        send(sockFd, cmd.c_str(), cmd.size(), MSG_NOSIGNAL);

        if (head == "QUIT") { running = false; break; }
    }

    running = false;
    shutdown(sockFd, SHUT_RDWR);
    close(sockFd);
    if (recvThread.joinable()) recvThread.join();

    clearScreen();
    std::cout << BOLD << WHITE << "\nGoodbye! 👋\n" << RESET;
    return 0;
}
