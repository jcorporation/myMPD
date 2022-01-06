/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2022 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#ifndef MYMPD_STATE_H
#define MYMPD_STATE_H

#include "../dist/rax/rax.h"
#include "../dist/sds/sds.h"
#include "list.h"

#include <mpd/client.h>
#include <time.h>

enum jukebox_modes {
    JUKEBOX_OFF,
    JUKEBOX_ADD_SONG,
    JUKEBOX_ADD_ALBUM,
};

enum trigger_events {
    TRIGGER_MYMPD_SCROBBLE = -1,
    TRIGGER_MYMPD_START = -2,
    TRIGGER_MYMPD_STOP = -3,
    TRIGGER_MYMPD_CONNECTED = -4,
    TRIGGER_MYMPD_DISCONNECTED = -5,
    TRIGGER_MPD_DATABASE = 0x1,
    TRIGGER_MPD_STORED_PLAYLIST = 0x2,
    TRIGGER_MPD_PLAYLIST = 0x4,
    TRIGGER_MPD_PLAYER = 0x8,
    TRIGGER_MPD_MIXER = 0x10,
    TRIGGER_MPD_OUTPUT = 0x20,
    TRIGGER_MPD_OPTIONS = 0x40,
    TRIGGER_MPD_UPDATE = 0x80,
    TRIGGER_MPD_STICKER = 0x100,
    TRIGGER_MPD_SUBSCRIPTION = 0x200,
    TRIGGER_MPD_MESSAGE = 0x400,
    TRIGGER_MPD_PARTITION = 0x800,
    TRIGGER_MPD_NEIGHBOR = 0x1000,
    TRIGGER_MPD_MOUNT = 0x2000
};

enum mpd_conn_states {
    MPD_DISCONNECTED,
    MPD_FAILURE,
    MPD_CONNECTED,
    MPD_RECONNECT,
    MPD_DISCONNECT_INSTANT,
    MPD_DISCONNECT,
    MPD_WAIT,
    MPD_TOO_OLD
};

struct t_sticker {
    unsigned playCount;
    unsigned skipCount;
    unsigned lastPlayed;
    unsigned lastSkipped;
    unsigned like;
};

struct t_tags {
    size_t len;
    enum mpd_tag_type tags[64];
};

struct t_mpd_state {
    //Connection
    struct mpd_connection *conn;
    enum mpd_conn_states conn_state;
    int mpd_timeout;
    bool mpd_keepalive;
    time_t reconnect_time;
    unsigned reconnect_interval;
    //connection configuration
    enum mpd_state state;
    sds mpd_host;
    int mpd_port;
    sds mpd_pass;
    unsigned mpd_binarylimit;
    //connection states
    int song_id;
    int next_song_id;
    int last_song_id;
    unsigned song_pos;
    sds song_uri;
    sds last_song_uri;
    long queue_version;
    long queue_length;
    int last_last_played_id;
    int last_skipped_id;
    time_t song_end_time;
    time_t last_song_end_time;
    time_t song_start_time;
    time_t last_song_start_time;
    time_t crossfade;
    time_t set_song_played_time;
    time_t last_song_set_song_played_time;
    //tags
    sds tag_list;
    struct t_tags tag_types_mympd;
    struct t_tags tag_types_mpd;
    unsigned tag_albumartist;
    //Feats
    const unsigned* protocol;
    bool feat_mpd_library;
    bool feat_mpd_tags;
    bool feat_mpd_advsearch;
    bool feat_mpd_stickers;
    bool feat_mpd_playlists;
    bool feat_mpd_fingerprint;
    bool feat_mpd_albumart;
    bool feat_mpd_readpicture;
    bool feat_mpd_single_oneshot;
    bool feat_mpd_mount;
    bool feat_mpd_neighbor;
    bool feat_mpd_partitions;
    bool feat_mpd_binarylimit;
    bool feat_mpd_smartpls;
    bool feat_mpd_playlist_rm_range;
    bool feat_mpd_whence;
};

struct t_timer_definition {
    sds name;
    bool enabled;
    int start_hour;
    int start_minute;
    sds action;
    sds subaction;
    int volume;
    sds playlist;
    unsigned jukebox_mode;
    bool weekdays[7];
    struct t_list arguments;
};

typedef void (*time_handler)(struct t_timer_definition *definition, void *user_data);

struct t_timer_node {
    int fd;
    time_handler callback;
    struct t_timer_definition *definition;
    void *user_data;
    unsigned timeout;
    int interval;
    int timer_id;
    struct t_timer_node *next;
};

struct t_timer_list {
    int length;
    int last_id;
    int active;
    struct t_timer_node *list;
};

struct t_mympd_state {
    //static config
    struct t_config *config;
    //mpd state
    struct t_mpd_state *mpd_state;
    //lists
    struct t_timer_list timer_list;
    struct t_list home_list;
    struct t_list triggers;
    struct t_list last_played;
    struct t_list jukebox_queue;
    struct t_list jukebox_queue_tmp;
    //caches
    rax *sticker_cache;
    struct t_list sticker_queue;
    bool sticker_cache_building;
    rax *album_cache;
    bool album_cache_building;
    //states - configurable with webui
    sds tag_list_search;
    sds tag_list_browse;
    struct t_tags tag_types_search;
    struct t_tags tag_types_browse;
    bool smartpls;
    sds smartpls_sort;
    sds smartpls_prefix;
    time_t smartpls_interval;
    struct t_tags smartpls_generate_tag_types;
    sds smartpls_generate_tag_list;
    unsigned last_played_count;
    bool auto_play;
    enum jukebox_modes jukebox_mode;
    sds jukebox_playlist;
    long jukebox_queue_length;
    long jukebox_last_played;
    struct t_tags jukebox_unique_tag;
    bool jukebox_enforce_unique;
    sds cols_queue_current;
    sds cols_search;
    sds cols_browse_database_detail;
    sds cols_browse_playlists_detail;
    sds cols_browse_filesystem;
    sds cols_playback;
    sds cols_queue_last_played;
    sds cols_queue_jukebox;
    sds cols_browse_radio_webradiodb;
    sds cols_browse_radio_radiobrowser;
    bool localplayer;
    int mpd_stream_port;
    sds music_directory;
    sds music_directory_value;
    sds playlist_directory;
    sds booklet_name;
    sds navbar_icons;
    sds coverimage_names;
    unsigned volume_min;
    unsigned volume_max;
    unsigned volume_step;
    sds lyrics_uslt_ext;
    sds lyrics_sylt_ext;
    sds lyrics_vorbis_uslt;
    sds lyrics_vorbis_sylt;
    int covercache_keep_days;
    //settings only for webui
    sds webui_settings;
};

#endif
