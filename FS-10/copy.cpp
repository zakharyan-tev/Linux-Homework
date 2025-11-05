#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <iostream>
#include <cstdlib>
#include <cstring>

constexpr size_t BUFFER_SIZE = 4096;

bool isHole(const char* buf, ssize_t len) {
    for (ssize_t i = 0; i < len; ++i) {
        if (buf[i] != '\0') return false;
    }
    return true;
}

int main(int argc, char* argv[]) {
    if (argc != 3) {
        std::cerr << "Usage: " << argv[0] << " <source_file> <destination_file>\n";
        return EXIT_FAILURE;
    }

    const char* sourceFile = argv[1];
    const char* destFile = argv[2];

    int fdSource = open(sourceFile, O_RDONLY);
    if (fdSource == -1) {
        std::perror("Cannot open source file");
        return EXIT_FAILURE;
    }

    int fdDest = open(destFile, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fdDest == -1) {
        std::perror("Cannot open destination file");
        close(fdSource);
        return EXIT_FAILURE;
    }

    char buf[BUFFER_SIZE];
    ssize_t readBytes;
    off_t totalRead = 0;
    off_t totalWrittenData = 0;
    off_t totalHoleBytes = 0;

    while ((readBytes = read(fdSource, buf, BUFFER_SIZE)) > 0) {
        if (isHole(buf, readBytes)) {
            if (lseek(fdDest, readBytes, SEEK_CUR) == -1) {
                std::perror("Seeking error in destination file");
                close(fdSource);
                close(fdDest);
                return EXIT_FAILURE;
            }
            totalHoleBytes += readBytes;
        } else {
            ssize_t written = 0;
            while (written < readBytes) {
                ssize_t result = write(fdDest, buf + written, readBytes - written);
                if (result == -1) {
                    std::perror("Writing error to destination file");
                    close(fdSource);
                    close(fdDest);
                    return EXIT_FAILURE;
                }
                written += result;
            }
            totalWrittenData += written;
        }
        totalRead += readBytes;
    }

    if (readBytes < 0) {
        std::perror("Error reading source file");
        close(fdSource);
        close(fdDest);
        return EXIT_FAILURE;
    }

    close(fdSource);
    close(fdDest);

    std::cout << "Copy completed: " << totalRead
              << " bytes (data: " << totalWrittenData
              << ", holes: " << totalHoleBytes << ").\n";

    return EXIT_SUCCESS;
}
