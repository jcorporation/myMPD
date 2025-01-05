/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2025 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

/*! \file
 * \brief Event handling
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
}

/**
 * Adds an fd to poll
 * @param pfds struct to add the fd
 * @param fd fd to add
 * @param type fd type to add
 * @param partition_state pointer to partition_state or NULL
 * @return true on success, else false
 */
bool event_pfd_add_fd(struct mympd_pfds *pfds, int fd, enum pfd_type type, struct t_partition_state *partition_state) {
    if (fd == -1) {
        return false;
    }
    if (pfds->len == POLL_FDS_MAX) {
        MYMPD_LOG_ERROR(NULL, "Too many file descriptors");
        return false;
    }
    pfds->fds[pfds->len].fd = fd;
    pfds->fds[pfds->len].events = POLLIN;
    pfds->fd_types[pfds->len] = type;
    pfds->partition_states[pfds->len] = partition_state;
    pfds->len++;
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
        MYMPD_LOG_ERROR(NULL, "Unable to create eventfd");
        MYMPD_LOG_ERRNO(NULL, errno);
    }
    return fd;
}

/**
 * Increments the eventfd by one
 * @param fd fd to write
 * @return true on success, else false
 */
bool event_eventfd_write(int fd) {
    errno = 0;
    if (eventfd_write(fd, 1) != 0) {
        MYMPD_LOG_ERROR(NULL, "Unable to write to eventfd");
        MYMPD_LOG_ERRNO(NULL, errno);
        return false;
    }
    return true;
}

/**
 * Reads from an eventfd
 * @param fd read from this fd
 * @return true on success, else false
 */
bool event_eventfd_read(int fd) {
    eventfd_t exp;
    errno = 0;
    if (eventfd_read(fd, &exp) != 0) {
        MYMPD_LOG_ERROR(NULL, "Unable to read from eventfd");
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

/**
 * Lookups the name for a pfd_type
 * @param type pfd_type
 * @return name as string
 */
const char *lookup_pfd_type(enum pfd_type type) {
    switch(type) {
        case PFD_TYPE_PARTITION: return "partition socket";
        case PFD_TYPE_STICKERDB: return "stickerdb socket";
        case PFD_TYPE_TIMER: return "general timer";
        case PFD_TYPE_QUEUE: return "queue event";
        case PFD_TYPE_TIMER_MPD_CONNECT: return "connect timer";
        case PFD_TYPE_TIMER_SCROBBLE: return "scrobble timer";
        case PFD_TYPE_TIMER_JUKEBOX: return "jukebox timer";
    }
    return "invalid";
}

/**
 * Lookups the name for poll revents
 * @param revent poll return event
 * @return name as string
 */
const char *lookup_pfd_revents(short revent) {
    if (revent & POLLIN) { return "POLLIN"; }
    if (revent & POLLPRI) { return "POLLPRI"; }
    if (revent & POLLOUT) { return "POLLOUT"; }
    if (revent & POLLHUP) { return "POLLHUP"; }
    if (revent & POLLERR) { return "POLLERR"; }
    if (revent & POLLNVAL) { return "POLLNVAL"; }
    return "";
}
