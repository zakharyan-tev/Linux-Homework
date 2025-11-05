#include <fcntl.h>   
#include <unistd.h>  
#include <stdlib.h>  
#include <stdio.h>  

int main(int argc, char** argv) {
    if (argc != 2) {
        fprintf(stderr, "Wrong arguments: Expected 1 argument, but got %d\n", argc - 1);
        exit(EXIT_FAILURE);
    }

    const char* filePath = argv[1];
	
    int fd = open(filePath, O_RDONLY);
    if (fd == -1) {
        perror("Error opening file");
        exit(EXIT_FAILURE);
    }

    const size_t bufferSize = 1024;
    char buffer[bufferSize];
    ssize_t bytesRead;

    while ((bytesRead = read(fd, buffer, bufferSize)) > 0) {
        ssize_t bytesWritten = write(1, buffer, bytesRead);
        if (bytesWritten == -1) {
            perror("Error writing to stdout");
            close(fd);
            exit(EXIT_FAILURE);
        }
    }

    if (bytesRead == -1) {
        perror("Error reading file");
    }

    close(fd);
    return 0;
}
