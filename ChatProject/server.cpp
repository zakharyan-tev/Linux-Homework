#include <iostream>
#include <vector>
#include <thread>
#include <mutex>
#include <map>
#include <set>
#include <algorithm>
#include <sstream>
#include <unistd.h>
#include <arpa/inet.h>
#include <cstring>

//  Encode / Decode  (length-prefix: "4#len#data")
std::string encode(const std::vector<std::string>& parts) {
    std::string out;
    for (const auto& s : parts) {
        out += std::to_string(s.size()) + "#" + s;
    }
    return out;
}

std::vector<std::string> decode(const std::string& raw, size_t& consumed) {
    std::vector<std::string> result;
    size_t pos = 0;
    while (pos < raw.size()) {
        size_t hash = raw.find('#', pos);
        if (hash == std::string::npos) break;
        size_t len = std::stoul(raw.substr(pos, hash - pos));
        size_t start = hash + 1;
        if (start + len > raw.size()) break;
        result.push_back(raw.substr(start, len));
        pos = start + len;
    }
    consumed = pos;
    return result;
}

struct Room {
    std::string name;
    bool is_private;
    std::string code;
    std::set<int> member_socks;
};

struct Client {
    int sock;
    std::string name;
    std::string room;
    std::string recv_buf;
};

std::map<std::string, Room> rooms;
std::map<int, Client> clients;
std::mutex global_mutex;

void send_raw(int sock, const std::string& msg) {
    std::string frame = encode({msg});
    send(sock, frame.c_str(), frame.size(), 0);
}

void send_to_room(const std::string& room_name, const std::string& msg,
                  int exclude_sock = -1) {
    auto it = rooms.find(room_name);
    if (it == rooms.end()) return;
    for (int s : it->second.member_socks) {
        if (s != exclude_sock) send_raw(s, msg);
    }
}

void leave_room(int sock) {
    auto& c = clients[sock];
    if (c.room.empty()) return;
    auto& r = rooms[c.room];
    r.member_socks.erase(sock);
    if (r.member_socks.empty()) {
       rooms.erase(c.room);
    }
    std::string msg = "[" + c.room + "] " + c.name + " left the room.";
    send_to_room(c.room, msg, sock);
    std::cout << msg << "\n";
    c.room = "";
}

void join_room(int sock, const std::string& room_name) {
    auto& c = clients[sock];
    auto& r = rooms[room_name];
    c.room = room_name;
    r.member_socks.insert(sock);
    std::string msg = "[" + room_name + "] " + c.name + " joined the room.";
    send_to_room(room_name, msg, sock);
    send_raw(sock, "You joined room: " + room_name);
    std::cout << msg << "\n";
}

std::string rooms_list_msg() {
    std::string out = "=== Rooms ===\n";
    if (rooms.empty()) { out += "  (none)\n"; return out; }
    for (auto& kv : rooms) {
        const Room& r = kv.second;
        out += "  " + kv.first
             + (r.is_private ? " [private]" : " [public]")
             + "  members: " + std::to_string(r.member_socks.size()) + "\n";
    }
    return out;
}

void handle_command(int sock, const std::string& line) {
    std::istringstream ss(line);
    std::string cmd;
    ss >> cmd;

    auto& c = clients[sock];

    if (cmd == "/rooms") {
        send_raw(sock, rooms_list_msg());
        return;
    }

    if (cmd == "/create") {
        std::string name, priv, code;
        ss >> name;
        if (name.empty()) { send_raw(sock, "Usage: /create <name> [private <code>]"); return; }
        if (rooms.count(name)) { send_raw(sock, "Room already exists."); return; }
        ss >> priv;
        bool is_priv = (priv == "private");
        if (is_priv) ss >> code;
        rooms[name] = {name, is_priv, code, {}};
        send_raw(sock, "Room \"" + name + "\" created" +
                 (is_priv ? " (private, code: " + code + ")" : " (public)") + ".");
        leave_room(sock);
        join_room(sock, name);
        return;
    }

    if (cmd == "/join") {
        std::string name, code;
        ss >> name;
        if (name.empty()) { send_raw(sock, "Usage: /join <name> [code]"); return; }
        auto it = rooms.find(name);
        if (it == rooms.end()) { send_raw(sock, "Room not found. Use /rooms to list."); return; }
        if (it->second.is_private) {
            ss >> code;
            if (code != it->second.code) { send_raw(sock, "Wrong code."); return; }
        }
        leave_room(sock);
        join_room(sock, name);
        return;
    }

    if (cmd == "/leave") {
        if (c.room.empty()) { send_raw(sock, "You are not in any room."); return; }
        leave_room(sock);
        send_raw(sock, "You left the room. Use /rooms and /join to enter another.");
        return;
    }

    if (cmd == "/pm") {
        std::string target;
        ss >> target;
        std::string text;
        std::getline(ss, text);
        if (text.size() && text[0] == ' ') text = text.substr(1);
        if (target.empty() || text.empty()) { send_raw(sock, "Usage: /pm <name> <message>"); return; }
        for (auto& kv : clients) {
            Client& cl = kv.second;
            if (cl.name == target) {
                send_raw(kv.first, "[PM from " + c.name + "] " + text);
                send_raw(sock, "[PM to " + target + "] " + text);
                return;
            }
        }
        send_raw(sock, "User \"" + target + "\" not found.");
        return;
    }

    if (cmd == "/who") {
        std::string out = "=== Online ===\n";
        for (auto& kv : clients) {
            const Client& cl = kv.second;
            out += "  " + cl.name + (cl.room.empty() ? " (lobby)" : " [" + cl.room + "]") + "\n";
        }
        send_raw(sock, out);
        return;
    }

    if (cmd == "/help") {
        send_raw(sock,
            "Commands:\n"
            "  /rooms                   – list all rooms\n"
            "  /who                     – list online users\n"
            "  /create <n> [private <code>]  – create a room\n"
            "  /join <n> [code]         – join a room\n"
            "  /leave                   – leave current room\n"
            "  /pm <name> <msg>         – private message\n"
            "  /help                    – this help\n"
        );
        return;
    }

    send_raw(sock, "Unknown command. Type /help for help.");
}

void handle_client(int sock) {
    char buf[4096];
    std::string accum;

    while (true) {
        int n = recv(sock, buf, sizeof(buf), 0);
        if (n <= 0) { close(sock); return; }
        accum += std::string(buf, n);
        size_t consumed = 0;
        auto parts = decode(accum, consumed);
        if (!parts.empty()) {
            std::string name = parts[0];
            accum = accum.substr(consumed);
            {
                std::lock_guard<std::mutex> lock(global_mutex);
                clients[sock] = {sock, name, "", accum};
            }
            send_raw(sock, "Welcome, " + name + "! Type /help for commands.");
            std::cout << name << " connected.\n";
            break;
        }
    }

    while (true) {
        int n = recv(sock, buf, sizeof(buf), 0);
        if (n <= 0) break;

        {
            std::lock_guard<std::mutex> lock(global_mutex);
            clients[sock].recv_buf += std::string(buf, n);
            size_t consumed = 0;
            auto parts = decode(clients[sock].recv_buf, consumed);
            clients[sock].recv_buf = clients[sock].recv_buf.substr(consumed);

            for (const auto& line : parts) {
                if (!line.empty() && line[0] == '/') {
                    handle_command(sock, line);
                } else {
                    const std::string cur_room = clients[sock].room;
                    const std::string cur_name = clients[sock].name;
                    if (cur_room.empty()) {
                        send_raw(sock, "You are not in a room. Use /join <room> or /create <room>.");
                    } else {
                        std::string full = "[" + cur_room + "] " + cur_name + ": " + line;
                        std::cout << full << "\n";
                        send_to_room(cur_room, full, sock);
                    }
                }
            }
         }
      }
    {
        std::lock_guard<std::mutex> lock(global_mutex);
        std::string name = clients[sock].name;
        leave_room(sock);
        clients.erase(sock);
        std::cout << name << " disconnected.\n";
    }
    close(sock);
}

int main() {
    int server_sock = socket(AF_INET, SOCK_STREAM, 0);
    int opt = 1;
    setsockopt(server_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    sockaddr_in addr{};
    addr.sin_family      = AF_INET;
    addr.sin_port        = htons(12345);
    addr.sin_addr.s_addr = INADDR_ANY;

    bind(server_sock, (sockaddr*)&addr, sizeof(addr));
    listen(server_sock, 32);

    std::cout << "Chat server running on :12345\n";

    while (true) {
        int client_sock = accept(server_sock, nullptr, nullptr);
        if (client_sock < 0) continue;
        std::thread(handle_client, client_sock).detach();
    }

    close(server_sock);
}


