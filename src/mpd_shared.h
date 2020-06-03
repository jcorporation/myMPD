/*
 SPDX-License-Identifier: GPL-2.0-or-later
 myMPD (c) 2018-2020 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#ifndef __MPD_SHARED_H__
#define __MPD_SHARED_H__

enum mpd_conn_states {
    MPD_DISCONNECTED,
    MPD_FAILURE,
    MPD_CONNECTED,
    MPD_RECONNECT,
    MPD_DISCONNECT,
    MPD_WAIT
};

typedef struct t_mpd_state {
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
} t_mpd__state;

void free_mpd_state(t_mpd_state *mpd_state);
void default_mpd_state(t_mpd_state *mpd_state);
void mpd_client_disconnect(t_mpd_state *mpd_state);
#endif
