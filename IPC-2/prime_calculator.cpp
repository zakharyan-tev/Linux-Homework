#include <iostream>
#include <unistd.h>
#include <fcntl.h>
#include <vector>
#include <sys/wait.h>
#include <cstdlib>
#include <cerrno>
#include <cstring>

bool checkIfPrime(int n) {
    if (n < 2) return false;
    if (n == 2 || n == 3) return true;
    if (n % 2 == 0 || n % 3 == 0) return false;
    for (int i = 5; i * i <= n; i += 6) {
        if (n % i == 0 || n % (i + 2) == 0) return false;
    }
    return true;
}

void addPrimesToArr(int n, std::vector<int> &arr) {
    for (int i = arr.back() + 1; arr.size() < n; ++i) {
        if (checkIfPrime(i)) {
            arr.push_back(i);
        }
    }
}

ssize_t safeWrite(int fd, const void* buf, size_t count) {
    ssize_t r = write(fd, buf, count);
    if (r == -1) {
        std::cerr << "write() failed: " << strerror(errno) << "\n";
        exit(EXIT_FAILURE);
    }
    return r;
}

ssize_t safeRead(int fd, void* buf, size_t count) {
    ssize_t r = read(fd, buf, count);
    if (r == -1) {
        std::cerr << "read() failed: " << strerror(errno) << "\n";
        exit(EXIT_FAILURE);
    }
    return r;
}

int main() {
    int rwPipefd[2];
    int wrPipefd[2];

    if (pipe(rwPipefd) == -1) {
        std::cerr << "Failed to create rwPipe: " << strerror(errno) << "\n";
        exit(EXIT_FAILURE);
    }
    if (pipe(wrPipefd) == -1) {
        std::cerr << "Failed to create wrPipe: " << strerror(errno) << "\n";
        exit(EXIT_FAILURE);
    }

    pid_t child = fork();
    if (child == -1) {
        std::cerr << "Failed to fork: " << strerror(errno) << "\n";
        exit(EXIT_FAILURE);
    }

    if (child == 0) {
        if (close(rwPipefd[0]) == -1 || close(wrPipefd[1]) == -1) {
            std::cerr << "[Child] close() failed: " << strerror(errno) << "\n";
            exit(EXIT_FAILURE);
        }

        std::vector<int> arr = {2, 3, 5, 7};

        while (true) {
            char buff[32];
            ssize_t rBytes = safeRead(wrPipefd[0], buff, sizeof(buff) - 1);
            if (rBytes == 0) break;

            buff[rBytes] = '\0';
            int n = std::atoi(buff);

            if (n <= 0 || n > 1000000) {
                std::string err = "[Child] Invalid number. Must be >0 and <=1,000,000\n";
                safeWrite(STDOUT_FILENO, err.c_str(), err.size());
                continue;
            }

            std::cout << "[Child] Calculating " << n << "-th prime number...\n";

            if ((int)arr.size() < n) {
                addPrimesToArr(n, arr);
            }

            std::string res = std::to_string(arr[n - 1]);
            safeWrite(rwPipefd[1], res.c_str(), res.size());
        }

        if (close(rwPipefd[1]) == -1 || close(wrPipefd[0]) == -1) {
            std::cerr << "[Child] close() failed: " << strerror(errno) << "\n";
        }

        exit(0);
    }
    
    if (close(rwPipefd[1]) == -1 || close(wrPipefd[0]) == -1) {
        std::cerr << "[Parent] close() failed: " << strerror(errno) << "\n";
        exit(EXIT_FAILURE);
    }

    while (true) {
        std::string m;
        std::cout << "[Parent] Please enter the number (or 'exit'): ";
        std::cin >> m;

        if (m == "exit") {
            if (close(wrPipefd[1]) == -1 || close(rwPipefd[0]) == -1) {
                std::cerr << "[Parent] close() failed: " << strerror(errno) << "\n";
            }
            wait(NULL);
            break;
        }

        int n = std::atoi(m.c_str());
        if (n <= 0 || n > 1000000) {
            std::cerr << "[Parent] Number must be >0 and <=1,000,000\n";
            continue;
        }

        std::cout << "[Parent] Sending " << n << " to the child process...\n";
        safeWrite(wrPipefd[1], m.c_str(), m.size());

        std::cout << "[Parent] Waiting for the response...\n";
        char buff[32];
        ssize_t rBytes = safeRead(rwPipefd[0], buff, sizeof(buff) - 1);
        if (rBytes == 0) {
            std::cerr << "[Parent] Child closed pipe unexpectedly.\n";
            break;
        }

        buff[rBytes] = '\0';
        std::cout << "[Parent] Received calculation result of prime(" << n << ") = "
                  << buff << "\n\n";
    }

    return 0;
}
