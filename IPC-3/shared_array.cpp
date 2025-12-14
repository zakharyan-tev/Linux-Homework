#include "shared_array.h"

#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <stdexcept>
#include <cstring>

shared_array::shared_array(const std::string& name, size_t size)
    : m_name(name), m_size(size)
{
    if (size == 0)
        throw std::runtime_error("Size must be > 0");

    m_shm_name = "/" + name + "_shm_" + std::to_string(size);
    m_sem_name = "/" + name + "_sem_" + std::to_string(size);

    m_fd = shm_open(m_shm_name.c_str(), O_CREAT | O_RDWR, 0666);
    if (m_fd < 0)
        throw std::runtime_error("shm_open failed");

    if (ftruncate(m_fd, total_bytes()) != 0)
        throw std::runtime_error("ftruncate failed");

    m_addr = mmap(nullptr, total_bytes(),
                  PROT_READ | PROT_WRITE, MAP_SHARED, m_fd, 0);
    if (m_addr == MAP_FAILED)
        throw std::runtime_error("mmap failed");

    m_data = reinterpret_cast<int*>(
        static_cast<char*>(m_addr) + sizeof(sem_t));

    m_sem = sem_open(m_sem_name.c_str(), O_CREAT, 0666, 1);
    if (m_sem == SEM_FAILED)
        throw std::runtime_error("sem_open failed");
}

shared_array::~shared_array()
{
    munmap(m_addr, total_bytes());
    close(m_fd);
    sem_close(m_sem);
}

size_t shared_array::total_bytes() const
{
    return sizeof(sem_t) + sizeof(int) * m_size;
}

int& shared_array::operator[](size_t index)
{
    if (index >= m_size)
        throw std::out_of_range("Index out of range");
    return m_data[index];
}

void shared_array::lock()
{
    sem_wait(m_sem);
}

void shared_array::unlock()
{
    sem_post(m_sem);
}
