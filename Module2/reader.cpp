#include <iostream>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <pthread.h>

struct SharedData {
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    bool ready;
    char message[256];
};

int main() {
    int fd = shm_open("/my_shm", O_RDWR, 0666);
    if (fd == -1) {
        perror("shm_open failed");
        return 1;
    }

    SharedData* data = (SharedData*)mmap(nullptr, sizeof(SharedData),
                                         PROT_READ | PROT_WRITE,
                                         MAP_SHARED, fd, 0);
    if (data == MAP_FAILED) {
        perror("mmap failed");
        return 1;
    }

    pthread_mutex_lock(&data->mutex);
    while (!data->ready) {
        pthread_cond_wait(&data->cond, &data->mutex);
    }

    std::cout << "Received: " << data->message << std::endl;
    pthread_mutex_unlock(&data->mutex);

    munmap(data, sizeof(SharedData));
    return 0;
}
