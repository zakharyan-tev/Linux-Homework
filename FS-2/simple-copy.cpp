#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

int main(int argc, char* argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <source> <destination>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    const char* sourcePath = argv[1];
    const char* destPath   = argv[2];

    int src = open(sourcePath, O_RDONLY);
    if (src == -1) {
        perror("Error: Cannot open source file");
        exit(EXIT_FAILURE);
    }

    int dst = open(destPath, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (dst == -1) {
        perror("Error: Cannot open destination file");
        close(src);
        exit(EXIT_FAILURE);
    }

    const size_t bufferSize = 4096;
    char buffer[bufferSize];
    ssize_t bytesRead;

    while ((bytesRead = read(src, buffer, bufferSize)) > 0) {
        ssize_t bytesWritten = write(dst, buffer, bytesRead);
        if (bytesWritten == -1) {
            perror("Error writing to destination file");
            close(src);
            close(dst);
            exit(EXIT_FAILURE);
        }
    }

    if (bytesRead == -1) {
        perror("Error reading source file");
    }

    close(src);
    close(dst);

    return 0;
}
