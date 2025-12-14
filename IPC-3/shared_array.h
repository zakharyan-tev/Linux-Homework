#ifndef SHARED_ARRAY_H
#define SHARED_ARRAY_H

#include <string>
#include <semaphore.h>

class shared_array {
public:
    shared_array(const std::string& name, size_t size);
    ~shared_array();

    int& operator[](size_t index);

    void lock();
    void unlock();

    size_t size() const { return m_size; }

private:
    std::string m_name;
    std::string m_shm_name;
    std::string m_sem_name;

    size_t m_size;
    int m_fd;
    void* m_addr;
    int* m_data;
    sem_t* m_sem;

    size_t total_bytes() const;
};

#endif
