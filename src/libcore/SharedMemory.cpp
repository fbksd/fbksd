#include "fbksd/core/SharedMemory.h"
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/statvfs.h>
#include <iostream>


SharedMemory::SharedMemory():
    m_fd(-1),
    m_size(0),
    m_errno(0),
    m_isCreator(false),
    m_isAttached(false),
    m_mem(MAP_FAILED)
{}

SharedMemory::SharedMemory(const std::string& key):
    m_key(key),
    m_fd(-1),
    m_size(0),
    m_errno(0),
    m_isCreator(false),
    m_isAttached(false),
    m_mem(MAP_FAILED)
{}

SharedMemory::~SharedMemory()
{
    if(m_isAttached)
        detach();

    if(m_isCreator)
        shm_unlink(m_key.c_str());
}

void SharedMemory::setKey(const std::string &key)
{
    m_key = key;
}

std::string SharedMemory::key()
{
    return m_key;
}

bool SharedMemory::create(size_t size)
{
    // check the physical memory size
    size_t nPages = sysconf(_SC_PHYS_PAGES);
    size_t pageSize = sysconf(_SC_PAGE_SIZE);
    size_t totalPhysMem = nPages * pageSize;
    if(size > totalPhysMem)
    {
        m_customError = "SheredMemory::create ERROR: requested size > total physical memory size";
        return false;
    }
    else
    {
        // check the /dev/shm size
        struct statvfs stat;
        if (statvfs("/dev/shm", &stat) != 0)
        {
            m_customError = "Couldn't determine shared memory partition size (/dev/shm)";
            return false;
        }
        size_t nPages = stat.f_blocks;
        size_t totalShmSize = nPages * pageSize;
        if(size > totalShmSize)
        {
            m_customError = "SheredMemory::create ERROR: requested size > total shared memory size";
            return false;
        }
    }

    m_size = size;
    m_fd = shm_open(m_key.c_str(), O_CREAT | O_RDWR, S_IRUSR | S_IWUSR);
    if(m_fd == -1)
    {
        m_errno = errno;
        return false;
    }

    if(ftruncate(m_fd, size) == -1)
    {
        m_errno = errno;
        return false;
    }

    m_isCreator = true;
    return attach();
}

bool SharedMemory::attach()
{
    if(!m_isAttached)
    {
        if(!m_isCreator)
        {
            m_fd = shm_open(m_key.c_str(), O_RDWR, S_IRUSR | S_IWUSR);
            if(m_fd == -1)
            {
                m_errno = errno;
                return false;
            }

            struct stat memStat;
            fstat(m_fd, &memStat);
            m_size = memStat.st_size;
        }

        m_mem = mmap(NULL, m_size, PROT_READ | PROT_WRITE, MAP_SHARED, m_fd, 0);
        if(m_mem == MAP_FAILED)
        {
            m_errno = errno;
            return false;
        }

        close(m_fd);
        m_isAttached = true;
    }

    return true;
}

bool SharedMemory::detach()
{
    if(m_isAttached)
    {
        if(munmap(m_mem, m_size) == 0)
        {
            m_mem = MAP_FAILED;
            m_isAttached = false;
            return true;
        }
        else
            m_errno = errno;
    }

    return false;
}

bool SharedMemory::isAttached()
{
    return m_isAttached;
}

void *SharedMemory::data()
{
    return m_mem != MAP_FAILED ? m_mem : nullptr;
}

std::string SharedMemory::error()
{
    if(m_customError != "")
        return m_customError;

    switch(m_errno)
    {
        case EACCES:
            return "EACCES";

        case EAGAIN:
            return "EAGAIN";

        case EBADF:
            return "EBADF";

        case EINVAL:
            return "EINVAL";

        case ENFILE:
            return "ENFILE";

        case ENODEV:
            return "ENODEV";

        case ENOMEM:
            return "ENOMEM";

        case EPERM:
            return "EPERM";

        case ETXTBSY:
            return "ETXTBSY";

        case EOVERFLOW:
            return "EOVERFLOW";

        default:
            return "";
    }
}

size_t SharedMemory::size()
{
    return m_size;
}
