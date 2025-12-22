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
    const char* name = "/my_shm";

    int fd = shm_open(name, O_CREAT | O_RDWR, 0666);
    ftruncate(fd, sizeof(SharedData));

    SharedData* data = (SharedData*)mmap(
        nullptr,
        sizeof(SharedData),
        PROT_READ | PROT_WRITE,
        MAP_SHARED,
        fd,
        0
    );

    pthread_mutexattr_t m_attr;
    pthread_condattr_t c_attr;

    pthread_mutexattr_init(&m_attr);
    pthread_mutexattr_setpshared(&m_attr, PTHREAD_PROCESS_SHARED);

    pthread_condattr_init(&c_attr);
    pthread_condattr_setpshared(&c_attr, PTHREAD_PROCESS_SHARED);

    pthread_mutex_init(&data->mutex, &m_attr);
    pthread_cond_init(&data->cond, &c_attr);

    data->ready = false;
    std::memset(data->message, 0, sizeof(data->message));

    std::cout << "Shared memory initialized\n";

    sleep(10);

    pthread_mutex_destroy(&data->mutex);
    pthread_cond_destroy(&data->cond);

    munmap(data, sizeof(SharedData));
    shm_unlink(name);

    return 0;
}
