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

typedef struct t_tags {
    size_t len;
    enum mpd_tag_type tags[64];
} t_tags;

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
    t_tags mympd_tag_types;
} t_mpd_state;

void mpd_shared_free_mpd_state(t_mpd_state *mpd_state);
void mpd_shared_default_mpd_state(t_mpd_state *mpd_state);
void mpd_shared_mpd_disconnect(t_mpd_state *mpd_state);
bool check_rc_error_and_recover(t_mpd_state *mpd_state, sds *buffer,
                                sds method, int request_id, bool notify, bool rc, const char *command);
bool check_error_and_recover2(t_mpd_state *mpd_state, sds *buffer, sds method, int request_id, bool notify);
sds check_error_and_recover(t_mpd_state *mpd_state, sds buffer, sds method, int request_id);
sds check_error_and_recover_notify(t_mpd_state *mpd_state, sds buffer);
sds respond_with_command_error(sds buffer, sds method, int request_id, const char *command);
sds respond_with_mpd_error_or_ok(t_mpd_state *mpd_state, sds buffer, sds method, int request_id, bool rc, const char *command);
void reset_t_tags(t_tags *tags);
void disable_all_mpd_tags(t_mpd_state *mpd_state);
void enable_all_mpd_tags(t_mpd_state *mpd_state);
void enable_mpd_tags(t_mpd_state *mpd_state, t_tags enable_tags);
#endif
