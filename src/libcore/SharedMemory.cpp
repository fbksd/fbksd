/*
 * Copyright (c) 2019 Jonas Deyson
 *
 * This software is released under the MIT License.
 *
 * You should have received a copy of the MIT License
 * along with this program. If not, see <https://opensource.org/licenses/MIT>
 */

#include "fbksd/core/SharedMemory.h"
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/statvfs.h>
#include <iostream>


SharedMemory::SharedMemory() = default;

SharedMemory::SharedMemory(const std::string& key):
    m_key(key)
{}

SharedMemory::SharedMemory(SharedMemory&& shm) noexcept
{
    *this = std::move(shm);
}

SharedMemory::~SharedMemory()
{
    if(m_isAttached)
        detach();

    if(m_isCreator)
        shm_unlink(m_key.c_str());
}

SharedMemory& SharedMemory::operator=(SharedMemory&& shm) noexcept
{
    m_key = std::move(shm.m_key);
    m_fd = shm.m_fd;
    m_size = shm.m_size;
    m_errno = shm.m_errno;
    m_isCreator = shm.m_isCreator;
    m_isAttached = shm.m_isAttached;
    m_customError = std::move(shm.m_customError);
    m_mem = shm.m_mem;

    shm.m_isCreator = false;
    shm.m_isAttached = false;
    shm.m_mem = nullptr;

    return *this;
}

void SharedMemory::setKey(const std::string &key)
{
    if(!m_isAttached)
        m_key = key;
}

const std::string& SharedMemory::key() const
{
    return m_key;
}

bool SharedMemory::create(size_t size)
{
    if(m_isAttached)
        return false;

    // check the physical memory size
    size_t nPages = static_cast<size_t>(sysconf(_SC_PHYS_PAGES));
    size_t pageSize = static_cast<size_t>(sysconf(_SC_PAGE_SIZE));
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
    if(!attach())
    {
        shm_unlink(m_key.c_str());
        return false;
    }
    return true;
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

        m_mem = mmap(nullptr, m_size, PROT_READ | PROT_WRITE, MAP_SHARED, m_fd, 0);
        if(m_mem == MAP_FAILED)
        {
            m_errno = errno;
            m_mem = nullptr;
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
            m_mem = nullptr;
            m_isAttached = false;
            return true;
        }
        else
            m_errno = errno;
    }

    return false;
}

bool SharedMemory::isAttached() const
{
    return m_isAttached;
}

const void *SharedMemory::data() const
{
    return m_mem;
}

void *SharedMemory::data()
{
    return m_mem;
}

std::string SharedMemory::error() const
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

size_t SharedMemory::size() const
{
    return m_size;
}
