/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2024 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#ifndef MYMPD_EVENT_H
#define MYMPD_EVENT_H

#include <poll.h>
#include <stdbool.h>

/**
 * Poll fd types
 */
enum pfd_type {
    PFD_TYPE_PARTITION,
    PFD_TYPE_STICKERDB,
    PFD_TYPE_TIMER,
    PFD_TYPE_QUEUE
};

/**
 * Struct holding the poll fds and there types
 */
struct mympd_pfds {
    struct pollfd fds[POLL_FDS_MAX];       //!< fds
    enum pfd_type fd_types[POLL_FDS_MAX];  //!< fd types
    nfds_t len;                            //!< number of mpd connection fds
    bool update_fds;                       //!< Update the fds array?
};

void event_pfd_init(struct mympd_pfds *pfds);
bool event_pfd_add_fd(struct mympd_pfds *pfds, int fd, enum pfd_type type);
bool event_pfd_read_fd(int fd);
int event_eventfd_create(void);
bool event_eventfd_write(int fd);
void event_fd_close(int fd);

#endif
