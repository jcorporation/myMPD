/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2024 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#ifndef MYMPD_EVENT_H
#define MYMPD_EVENT_H

#include "compile_time.h"
#include <poll.h>
#include <stdbool.h>

/**
 * Poll fd types
 */
enum pfd_type {
    /* MPD connection for partitions */
    PFD_TYPE_PARTITION = 0x1,
    /* MPD connection for stickerdb */
    PFD_TYPE_STICKERDB = 0x2,
    /* myMPD timers */
    PFD_TYPE_TIMER = 0x4,
    /* Message queue */
    PFD_TYPE_QUEUE = 0x8,
    /* Timer for mpd connect and reconnect */
    PFD_TYPE_TIMER_MPD_CONNECT = 0x10,
    /* Scrobble timer */
    PFD_TYPE_TIMER_SCROBBLE = 0x20,
    /* Jukebox timer */
    PFD_TYPE_TIMER_JUKEBOX = 0x40
};

/**
 * Struct holding the poll fds and there types
 */
struct mympd_pfds {
    struct pollfd fds[POLL_FDS_MAX];                           //!< fds
    enum pfd_type fd_types[POLL_FDS_MAX];                      //!< fd types
    nfds_t len;                                                //!< number of mpd connection fds
    struct t_partition_state *partition_states[POLL_FDS_MAX];  //!< pointer to partition_state
};

void event_pfd_init(struct mympd_pfds *pfds);
bool event_pfd_add_fd(struct mympd_pfds *pfds, int fd, enum pfd_type type, struct t_partition_state *partition_state);
bool event_pfd_read_fd(int fd);
int event_eventfd_create(void);
bool event_eventfd_write(int fd);
void event_fd_close(int fd);
const char *lookup_pfd_type(enum pfd_type type);
const char *lookup_pfd_revents(short revent);

#endif
