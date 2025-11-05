#include <iostream>
#include <unistd.h>
#include <sys/wait.h>
#include <ctime>
#include <cstdlib>

void do_command(char* argv[]) {
    if (!argv || !argv[0]) {
        std::cerr << "No command provided!\n";
        return;
    }

    std::time_t start = std::time(nullptr);

    pid_t pid = fork();

    if (pid == 0) {
        execvp(argv[0], argv);
        perror("execvp failed");
        std::exit(1);
    } else if (pid > 0) {
        int status;
        pid_t wait_pid = wait(&status);

        if (wait_pid == -1) {
            perror("wait failed");
            std::exit(1);
        }

        int exit_code = status >> 8;
        std::time_t end = std::time(nullptr);

        std::cout << "Command completed with " << exit_code
                  << " exit code and took " << (end - start) << " seconds.\n";
    } else {
        perror("fork failed");
        std::exit(1);
    }
}

int main(int argc, char* argv[]) {
    do_command(argv + 1);
    return 0;
}
