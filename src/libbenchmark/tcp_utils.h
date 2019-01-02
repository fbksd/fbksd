/*
 * Copyright (c) 2019 Jonas Deyson
 *
 * This software is released under the MIT License.
 *
 * You should have received a copy of the MIT License
 * along with this program. If not, see <https://opensource.org/licenses/MIT>
 */

#ifndef TCP_UTILS_H
#define TCP_UTILS_H

/**
 * @brief Waits for the given port to be in use.
 *
 * This can be used to wait for a server to open it's port.
 */
void waitPortOpen(unsigned short port);

#endif // TCP_UTILS_H
