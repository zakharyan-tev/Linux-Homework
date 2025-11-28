#include <iostream>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <cstdlib>
#include <string>
#include <sstream>

bool is_prime(int n) {
    if (n < 2) return false;
    for (int i = 2; i*i <= n; ++i)
        if (n % i == 0) return false;
    return true;
}

int nth_prime(int n) {
    int count = 0;
    int num = 1;
    while (count < n) {
        ++num;
        if (is_prime(num))
            ++count;
    }
    return num;
}

int main() {
    int pipe_parent_to_child[2];
    int pipe_child_to_parent[2];

    if (pipe(pipe_parent_to_child) == -1 || pipe(pipe_child_to_parent) == -1) {
        perror("pipe");
        return 1;
    }

    pid_t pid = fork();
    if (pid < 0) {
        perror("fork");
        return 1;
    }

    if (pid == 0) {
        close(pipe_parent_to_child[1]); 
        close(pipe_child_to_parent[0]); 

        while (true) {
            int m;
            if (read(pipe_parent_to_child[0], &m, sizeof(m)) <= 0) break;
            std::cout << "[Child] Calculating " << m << "-th prime number..." << std::endl;

            int result = nth_prime(m);

            std::cout << "[Child] Sending calculation result of prime(" << m << ")..." << std::endl;
            write(pipe_child_to_parent[1], &result, sizeof(result));
        }

        close(pipe_parent_to_child[0]);
        close(pipe_child_to_parent[1]);
        exit(0);
    } else {
        close(pipe_parent_to_child[0]);
        close(pipe_child_to_parent[1]); 

        std::string line;
        while (true) {
            std::cout << "[Parent] Please enter the number (or 'exit'): ";
            std::getline(std::cin, line);
            if (line == "exit") break;

            std::istringstream iss(line);
            int m;
            if (!(iss >> m)) {
                std::cout << "[Parent] Invalid input. Try again." << std::endl;
                continue;
            }

            std::cout << "[Parent] Sending " << m << " to the child process..." << std::endl;
            write(pipe_parent_to_child[1], &m, sizeof(m));

            std::cout << "[Parent] Waiting for the response from the child process..." << std::endl;
            int result;
            read(pipe_child_to_parent[0], &result, sizeof(result));
            std::cout << "[Parent] Received calculation result of prime(" << m << ") = " << result << "..." << std::endl;
        }

        close(pipe_parent_to_child[1]);
        close(pipe_child_to_parent[0]);

        wait(nullptr);
        std::cout << "[Parent] Exiting." << std::endl;
    }

    return 0;
}
