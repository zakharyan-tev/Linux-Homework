#include <iostream>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <pthread.h>
#include <cstring>

struct SharedData {
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    bool ready;
    char message[256];
};

int main() {
    int fd = shm_open("/my_shm", O_RDWR, 0666);

    SharedData* data = (SharedData*)mmap(
        nullptr,
        sizeof(SharedData),
        PROT_READ | PROT_WRITE,
        MAP_SHARED,
        fd,
        0
    );

    pthread_mutex_lock(&data->mutex);

    std::strcpy(data->message, "Hello from writer process!");
    data->ready = true;

    pthread_cond_signal(&data->cond);
    pthread_mutex_unlock(&data->mutex);

    munmap(data, sizeof(SharedData));
    return 0;
}
