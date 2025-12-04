#include <iostream>
#include <unistd.h>
#include <fcntl.h>
#include <vector>
#include <sys/wait.h>
#include <cstdlib>

bool checkIfPrime(int n) {
    if (n < 2) return false;
    if (n == 2 || n == 3) return true;
    if (n % 2 == 0 || n % 3 == 0) return false;
    for (int i = 5; i * i <= n; i += 6) {
        if (n % i == 0 || n % (i + 2) == 0){
            return false;
        }
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

int main() {
    int rwPipefd[2];
    int wrPipefd[2];

    if (pipe(rwPipefd) == -1 || pipe(wrPipefd) == -1) {
        std::cerr << "Failed to create pipes\n";
        exit(EXIT_FAILURE);
    }

    pid_t child = fork();

    if (child == -1) {
        std::cerr << "Failed to create a child\n";
        exit(EXIT_FAILURE);
    }

    if (child == 0) {
        close(rwPipefd[0]);
        close(wrPipefd[1]);

        std::vector<int> arr = {2, 3, 5, 7};

        while (true) {
            char buff[20];
            int rBytes = read(wrPipefd[0], buff, sizeof(buff) - 1);

            if (rBytes <= 0) {
                break;
            }
            
            buff[rBytes] = '\0';
            int n = std::atoi(buff);

            std::cout << "[Child] Calculating " << n << "-th prime number...\n";

            if (n > (int)arr.size()){
                addPrimesToArr(n, arr);
            }

            std::cout << "[Child] Sending calculation result of prime(" << n << ")...\n";

            std::string res = std::to_string(arr[n - 1]);

            write(rwPipefd[1], res.c_str(), res.size());
        }

        close(rwPipefd[1]);
        close(wrPipefd[0]);
        exit(0);
    }

    close(rwPipefd[1]);
    close(wrPipefd[0]);

    while (true) {
        std::string m;
        std::cout << "[Parent] Please enter the number: ";
        std::cin >> m;

        if (m == "exit") {
            close(wrPipefd[1]);
            close(rwPipefd[0]);
            wait(NULL);
            break;
        }

        std::cout << "[Parent] Sending " << m << " to the child process...\n";
        write(wrPipefd[1], m.c_str(), m.size());

        std::cout << "[Parent] Waiting for the response...\n";

        char buff[20];
        int rBytes = read(rwPipefd[0], buff, sizeof(buff) - 1);

        if (rBytes <= 0) {
            std::cerr << "[Parent] Child closed pipe unexpectedly.\n";
            break;
        }

        buff[rBytes] = '\0';
        std::cout << "[Parent] Received calculation result of prime(" << m 
                  << ") = " << buff << "...\n\n";
    }

    return 0;
}
