/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2023 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include "compile_time.h"
#include "src/lib/event.h"

#include "src/lib/log.h"

#include <errno.h>
#include <string.h>
#include <sys/eventfd.h>
#include <unistd.h>

/**
 * Initializes the mympd_pfds struct to zero
 * @param pfds struct to initialize
 */
void event_pfd_init(struct mympd_pfds *pfds) {
    memset(pfds, 0, sizeof(struct mympd_pfds));
    pfds->update_fds = true;
}

/**
 * Adds an fd to poll
 * @param pfds struct to add the fd
 * @param fd fd to add
 * @param type fd type to add
 * @return true on success, else false
 */
bool event_pfd_add_fd(struct mympd_pfds *pfds, int fd, enum pfd_type type) {
    if (pfds->len == POLL_FDS_MAX) {
        MYMPD_LOG_ERROR(NULL, "Too many file descriptors");
        return false;
    }
    pfds->fds[pfds->len].fd = fd;
    pfds->fds[pfds->len].events = POLLIN;
    pfds->fd_types[pfds->len] = type;
    pfds->len++;
    return true;
}

/**
 * Reads an uint64_t value from a fd
 * @param fd read from this fd
 * @return true on success, else false
 */
bool event_pfd_read_fd(int fd) {
    uint64_t exp;
    errno = 0;
    ssize_t s = read(fd, &exp, sizeof(uint64_t));
    if (s != sizeof(uint64_t)) {
        MYMPD_LOG_ERROR(NULL, "Unable to read from fd");
        MYMPD_LOG_ERRNO(NULL, errno);
        return false;
    }
    return true;
}

/**
 * Creates an eventfd
 * @return the eventfd
 */
int event_eventfd_create(void) {
    errno = 0;
    int fd = eventfd(0, EFD_CLOEXEC | EFD_NONBLOCK | EFD_SEMAPHORE);
    if (fd == -1) {
        MYMPD_LOG_ERROR(NULL, "Unable to write to fd");
        MYMPD_LOG_ERRNO(NULL, errno);
    }
    return fd;
}

/**
 * Writes the uint64_t value 1 to the fd
 * @param fd fd to write
 * @return true on success, else false
 */
bool event_eventfd_write(int fd) {
    errno = 0;
    uint64_t u = 1;
    if (write(fd, &u, sizeof(u)) != sizeof(u)) {
        MYMPD_LOG_ERROR(NULL, "Unable to write to fd");
        MYMPD_LOG_ERRNO(NULL, errno);
        return false;
    }
    return true;
}

/**
 * Closes the fd
 * @param fd fd to close
 */
void event_fd_close(int fd) {
    if (fd > -1) {
        close(fd);
    }
}
