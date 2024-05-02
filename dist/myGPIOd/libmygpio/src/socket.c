/*
 SPDX-License-Identifier: GPL-3.0-or-later
 libmygpio (c) 2020-2024 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include "libmygpio/src/socket.h"

#include <assert.h>
#include <fcntl.h>
#include <poll.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

/**
 * Connects to a socket
 * @param socket_path unix socket to connect
 * @return open file descriptor
 */
int libmygpio_socket_connect(const char *socket_path) {
    struct sockaddr_un address = { 0 };
    address.sun_family = AF_UNIX;
    strncpy(address.sun_path, socket_path, 108);

    int fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (fd == -1) {
        return -1;
    }

    int flags = fcntl(fd, F_GETFD, 0);
    if (fcntl(fd, F_SETFD, flags | O_CLOEXEC)) {
        close(fd);
        return -1;
    }

    if (connect(fd, (struct sockaddr *)&address, sizeof(address)) == -1) {
        close(fd);
        return -1;
    }
    return fd;
}

/**
 * Closes the file descriptor
 * @param fd file descriptor
 */
void libmygpio_socket_close(int fd) {
    if (fd > 0) {
        close(fd);
    }
}

/**
 * Receives a line from the socket and crops the ending LF.
 * This command blocks.
 * @param fd file descriptor to read
 * @param buf buffer to fill
 * @param timeout_ms timeout in ms
 *                   0 for no wait
 *                   -1 for no timeout
 * @return true on success, else false
 */
bool libmygpio_socket_recv_line(int fd, struct t_buf *buf, int timeout_ms) {
    libmygpio_buf_init(buf);
    ssize_t nread;
    int flag = 0;

    if (timeout_ms == 0) {
        flag = MSG_DONTWAIT;
    }
    else if (timeout_ms > 0) {
        struct pollfd pfds[1];
        pfds[0].fd = fd;
        pfds[0].events = POLLIN;
        if (poll(pfds, 1, timeout_ms) <= 0) {
            return false;
        }
    }

    while ((nread = recv(fd, buf->buffer + buf->len, 1, flag)) > 0) {
        buf->len += (size_t)nread;
        if (buf->buffer[buf->len - 1] == '\n') {
            buf->len--;
            buf->buffer[buf->len] = '\0';
            return true;
        }
        if (buf->len == BUFFER_SIZE_MAX) {
            break;
        }
    }
    buf->buffer[buf->len - 1] = '\0';
    return false;
}

/**
 * Writes a line to the socket.
 * Buffer will be cleared.
 * @param fd socket to write
 * @param buf buffer to write
 * @return true on success, else false
 */
bool libmygpio_socket_send_line(int fd, struct t_buf *buf) {
    ssize_t nwrite;
    size_t written = 0;
    size_t max_bytes = buf->len;
    while ((nwrite = write(fd, buf->buffer + written, max_bytes)) > 0) {
        if (nwrite < 0) {
            libmygpio_buf_init(buf);
            return false;
        }
        written += (size_t)nwrite;
        max_bytes = buf->len - written;
        if (written == buf->len) {
            libmygpio_buf_init(buf);
            if (write(fd, "\n", 1) != 1) {
                return false;
            }
            return true;
        }
    }
    libmygpio_buf_init(buf);
    return false;
}
