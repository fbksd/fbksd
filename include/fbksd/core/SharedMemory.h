/*
 * Copyright (c) 2018 Jonas Deyson
 *
 * This software is released under the MIT License.
 *
 * You should have received a copy of the MIT License
 * along with this program. If not, see <https://opensource.org/licenses/MIT>
 */

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
    /**
     * @brief Creates an empty shared memory (shm) object.
     *
     * An empty shm object is not attached.
     */
    SharedMemory();

    /**
     * @brief Creates an empty shm object with the given key.
     */
    SharedMemory(const std::string& key);

    SharedMemory(const SharedMemory&) = delete;

    SharedMemory(SharedMemory&&) noexcept;

    ~SharedMemory();

    SharedMemory& operator=(SharedMemory&&) noexcept;

    /**
     * @brief Sets the key.
     *
     * If this object is already attached, this method has no effect.
     */
    void setKey(const std::string& key);

    /**
     * @brief Returns the key.
     */
    const std::string& key() const;

    /**
     * @brief Creates and attaches a shm block with the given size.
     *
     * @returns true if it succeeded, false otherwise.
     */
    bool create(size_t size);

    /**
     * @brief Attaches this shm to an existing block.
     *
     * @returns true if it succeeded, false otherwise.
     */
    bool attach();

    /**
     * @brief Returns true if this shm object is attached.
     */
    bool isAttached() const;

    /**
     * @brief Detaches this shm object.
     */
    bool detach();

    /**
     * @brief Returns a pointer to the shm block.
     *
     * If this shm object is not attached, a nullptr is returned.
     */
    const void* data() const;

    /**
     * @brief Non-const overload for data().
     */
    void* data();

    /**
     * @brief Returns the size (in bytes) of the currently attached memory block.
     */
    size_t size() const;

    std::string error() const;

    SharedMemory& operator=(const SharedMemory&) = delete;

private:
    std::string m_key;
    int m_fd = -1;
    size_t m_size = 0;
    int m_errno = 0;
    bool m_isCreator = false;
    bool m_isAttached = false;
    std::string m_customError;
    void* m_mem = nullptr;
};

#endif // SHAREDMEMORY_H
