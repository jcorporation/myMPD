/*
 SPDX-License-Identifier: GPL-2.0-or-later
 myMPD (c) 2018-2020 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#ifndef __MPD_WORKER_UTILITY_H__
#define __MPD_WORKER_UTILITY_H__

typedef struct t_mpd_worker_state {
    // Connection
    struct mpd_connection *conn;
    enum mpd_conn_states conn_state;
    int timeout;
    time_t reconnect_time;
    unsigned reconnect_interval;
    // States
    enum mpd_state state;
    sds mpd_host;
    int mpd_port;
    sds mpd_pass;
} t_mpd_worker_state;

void free_mpd_worker_state(t_mpd_worker_state *mpd_worker_state);
void default_mpd_workerstate(t_mpd_worker_state *mpd_worker_state);
#endif
