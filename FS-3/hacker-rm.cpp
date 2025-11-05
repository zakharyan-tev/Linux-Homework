#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

int main(int argc, char** argv) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <file>\n", argv[0]);
        return EXIT_FAILURE;
    }

    const char* path = argv[1];

    int fd = open(path, O_RDWR);
    if (fd == -1) {
        perror("Error opening file");
        return EXIT_FAILURE;
    }

    off_t file_size = lseek(fd, 0, SEEK_END);
    if (file_size == -1) {
        perror("Error getting file size");
        close(fd);
        return EXIT_FAILURE;
    }

    if (lseek(fd, 0, SEEK_SET) == -1) {
        perror("Error seeking to beginning");
        close(fd);
        return EXIT_FAILURE;
    }

    const size_t BUF_SIZE = 4096;
    char buffer[BUF_SIZE];
    memset(buffer, 0, sizeof(buffer));

    off_t remaining = file_size;
    while (remaining > 0) {
        ssize_t to_write = (remaining > (off_t)BUF_SIZE) ? (ssize_t)BUF_SIZE : (ssize_t)remaining;
        ssize_t written = write(fd, buffer, to_write);
        if (written == -1) {
            perror("Error writing to file");
            close(fd);
            return EXIT_FAILURE;
        }
        remaining -= written;
    }

    if (fsync(fd) == -1) {
        perror("Warning: fsync failed");
        /* не фатально — продолжаем попытку удалить файл */
    }

    if (close(fd) == -1) {
        perror("Error closing file");
        return EXIT_FAILURE;
    }

    if (unlink(path) == -1) {
        perror("Error deleting file");
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
