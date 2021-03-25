/*
 SPDX-License-Identifier: GPL-2.0-or-later
 myMPD (c) 2018-2021 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#ifndef __MPD_SHARED_TYPEDEFS_H__
#define __MPD_SHARED_TYPEDEFS_H__

enum mpd_conn_states {
    MPD_DISCONNECTED,
    MPD_FAILURE,
    MPD_CONNECTED,
    MPD_RECONNECT,
    MPD_DISCONNECT,
    MPD_WAIT,
    MPD_TOO_OLD
};

typedef struct t_sticker {
    unsigned int playCount;
    unsigned int skipCount;
    unsigned int lastPlayed;
    unsigned int lastSkipped;
    unsigned int like;
} t_sticker;

typedef struct t_tags {
    size_t len;
    enum mpd_tag_type tags[64];
} t_tags;

typedef struct t_mpd_state {
    //Connection
    struct mpd_connection *conn;
    enum mpd_conn_states conn_state;
    int timeout;
    time_t reconnect_time;
    unsigned reconnect_interval;
    //States
    enum mpd_state state;
    sds mpd_host;
    int mpd_port;
    sds mpd_pass;
    //tags
    sds taglist;
    t_tags mympd_tag_types;
    t_tags mpd_tag_types;
    //Feats
    bool feat_tags;
    bool feat_advsearch;
    bool feat_stickers;
    bool feat_playlists;
} t_mpd_state;
#endif
