#ifndef TCP_UTILS_H
#define TCP_UTILS_H

/**
 * @brief Waits for the given port to be in use.
 *
 * This can be used to wait for a server to open it's port.
 */
void waitPortOpen(unsigned short port);

#endif // TCP_UTILS_H
