#include <iostream>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

void check(bool cond, const char* msg) {
    if (cond) {
        fprintf(stderr, "%s\n", msg);
        exit(EXIT_FAILURE);
    }
}

void check_sys(bool cond, const char* msg) {
    if (cond) {
        perror(msg);
        exit(EXIT_FAILURE);
    }
}

int main(int argc, char** argv)
{
    check(argc != 2, "Usage: program <file>");

    int fd = open(argv[1], O_WRONLY | O_CREAT | O_TRUNC, 0664);
    check_sys(fd == -1, "open");

    int fd2 = dup(fd);
    check_sys(fd2 == -1, "dup");

    const char str[] = "first line\n";
    const char str2[] = "second line\n";

    ssize_t n;
    n = write(fd, str, sizeof(str) - 1);
    check_sys(n == -1, "write fd");
    if (n != (ssize_t)(sizeof(str) - 1)) {
        fprintf(stderr, "Partial write to fd\n");
        /* при желании: обработать повторную запись */
    }

    n = write(fd2, str2, sizeof(str2) - 1);
    check_sys(n == -1, "write fd2");
    if (n != (ssize_t)(sizeof(str2) - 1)) {
        fprintf(stderr, "Partial write to fd2\n");
    }

    close(fd);
    close(fd2);

    return 0;
}
