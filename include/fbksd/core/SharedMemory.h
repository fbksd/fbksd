#ifndef SHAREDMEMORY_H
#define SHAREDMEMORY_H

#include <string>


/**
 * \brief The SharedMemory class provides shared memory access.
 *
 * This class uses the POSIX shared memory system.
 */
class SharedMemory
{
public:
    SharedMemory();
    SharedMemory(const std::string& key);
    ~SharedMemory();

    void setKey(const std::string& key);

    std::string key();

    bool create(size_t size);

    bool attach();

    bool isAttached();

    bool detach();

    void* data();

    std::string error();

    size_t size();

private:
    std::string m_key;
    int m_fd;
    size_t m_size;
    int m_errno;
    bool m_isCreator;
    bool m_isAttached;
    std::string m_customError;
    void* m_mem;
};

#endif // SHAREDMEMORY_H
