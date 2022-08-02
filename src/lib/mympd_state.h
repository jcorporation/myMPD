/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2022 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#ifndef MYMPD_STATE_H
#define MYMPD_STATE_H

#include "../dist/rax/rax.h"
#include "../dist/sds/sds.h"
#include "config_def.h"
#include "list.h"

#include <mpd/client.h>
#include <time.h>

/**
 * Jukebox state
 */
enum jukebox_modes {
    JUKEBOX_OFF,
    JUKEBOX_ADD_SONG,
    JUKEBOX_ADD_ALBUM,
    JUKEBOX_UNKNOWN
};

/**
 * myMPD trigger events. The list is composed of MPD idle events and
 * myMPD specific events.
 */
enum trigger_events {
    TRIGGER_MYMPD_SCROBBLE = -1,
    TRIGGER_MYMPD_START = -2,
    TRIGGER_MYMPD_STOP = -3,
    TRIGGER_MYMPD_CONNECTED = -4,
    TRIGGER_MYMPD_DISCONNECTED = -5,
    TRIGGER_MYMPD_FEEDBACK = -6,
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

/**
 * MPD connection states
 */
enum mpd_conn_states {
    MPD_CONNECTED,           //<! mpd is connected
    MPD_DISCONNECTED,        //<! mpd is disconnected, try to reconnect
    MPD_FAILURE,             //<! mpd connection failed, disconnect mpd and reconnect after wait time
    MPD_DISCONNECT,          //<! disconnect mpd and reconnect after wait time
    MPD_DISCONNECT_INSTANT,  //<! disconnect mpd and reconnect as soon as possible
    MPD_WAIT                 //<! wating for reconnection
};

/**
 * Sticker values
 */
struct t_sticker {
    long playCount;
    long skipCount;
    time_t lastPlayed;
    time_t lastSkipped;
    long like;
};

/**
 * Struct for mpd tag lists
 */
struct t_tags {
    size_t len;
    enum mpd_tag_type tags[64];
};

/**
 * Holds cache information
 */
struct t_cache {
    bool building;
    rax *cache;
};

/**
 * Holds MPD specific states shared across all partitions
 */
struct t_mpd_shared_state {
    //static config
    struct t_config *config;   
    //connection configuration
    sds mpd_host;
    unsigned mpd_port;
    sds mpd_pass;
    unsigned mpd_binarylimit;
    unsigned mpd_timeout;
    bool mpd_keepalive;
    //real music directory set by feature detection
    sds music_directory_value;
    //tags
    sds tag_list;
    struct t_tags tag_types_mympd;
    struct t_tags tag_types_mpd;
    struct t_tags tag_types_search;
    struct t_tags tag_types_browse;
    enum mpd_tag_type tag_albumartist;
    //Features
    const unsigned *protocol;
    bool feat_mpd_library;
    bool feat_mpd_tags;
    bool feat_mpd_stickers;
    bool feat_mpd_playlists;
    bool feat_mpd_fingerprint;
    bool feat_mpd_albumart;
    bool feat_mpd_readpicture;
    bool feat_mpd_mount;
    bool feat_mpd_neighbor;
    bool feat_mpd_partitions;
    bool feat_mpd_binarylimit;
    bool feat_mpd_playlist_rm_range;
    bool feat_mpd_whence;
    bool feat_mpd_advqueue;
    //caches
    struct t_cache album_cache;
    struct t_cache sticker_cache;
    //lists
    struct t_list last_played;
    long last_played_count;
    struct t_list sticker_queue;
    //name of the booklet files
    sds booklet_name;
};

/**
 * Holds partition specific states
 */
struct t_partition_state {
    //partition name
    sds name;
    //mpd connection
    struct mpd_connection *conn;
    enum mpd_conn_states conn_state;
    //reconnect timer
    time_t reconnect_time; //timestamp at which next connection attempt is made
    time_t reconnect_interval; //interval for connections attempts (increases by failed attempts)
    //track player states
    enum mpd_state play_state; //mpd player state
    int song_id; //current song id from queue
    int next_song_id; //next song id from queue
    int last_song_id; //previous song id from queue
    int song_pos; //current song pos in queue
    sds song_uri; //current song uri
    sds last_song_uri; // previous song uri
    unsigned queue_version; //queue version number (increments on queue change)
    long long queue_length; //length of the queue
    int last_last_played_id; //last scrobble event was fired for this song id
    int last_skipped_id; //last skipped event was fired for this song id
    time_t song_end_time; //timestamp at which current song should end (starttime + duration)
    time_t last_song_end_time; //timestamp at which previous song should end (starttime + duration)
    time_t song_start_time; //timestamp at which current song has started
    time_t last_song_start_time; //timestap at which previous song has started
    time_t crossfade; //used for determine when to add next song from jukebox queue
    time_t set_song_played_time; //timestamp at which the scrobble event will be fired
    time_t last_song_set_song_played_time; //timestamp of the previous scrobble event
    //start play if queue changes
    bool auto_play;
    //jukebox
    enum jukebox_modes jukebox_mode;
    sds jukebox_playlist;
    long jukebox_queue_length;
    long jukebox_last_played;
    struct t_tags jukebox_unique_tag;
    bool jukebox_enforce_unique;
    struct t_list jukebox_queue;
    struct t_list jukebox_queue_tmp;
    //pointer to shared MPD state
    struct t_mpd_shared_state *mpd_shared_state;
    //pointer to next partition;
    struct t_partition_state *next;
};

/**
 * Optional timer definition from GUI
 */
struct t_timer_definition {
    sds name;
    bool enabled;
    int start_hour;
    int start_minute;
    sds action;
    sds subaction;
    unsigned volume;
    sds playlist;
    enum jukebox_modes jukebox_mode;
    bool weekdays[7];
    struct t_list arguments;
};

/**
 * Callback functions for timers
 */
typedef void (*timer_handler)(int timer_id, struct t_timer_definition *definition);

/**
 * Timer node
 */
struct t_timer_node {
    int fd;
    timer_handler callback;
    struct t_timer_definition *definition;
    time_t timeout;
    int interval;
    int timer_id;
    struct t_timer_node *next;
};

/**
 * Linked list of timers containing t_timer_nodes
 */
struct t_timer_list {
    int length;
    int last_id;
    int active;
    struct t_timer_node *list;
};

/**
 * Lyrics settings
 */
struct t_lyrics {
    sds uslt_ext;
    sds sylt_ext;
    sds vorbis_uslt;
    sds vorbis_sylt;
};

/**
 * Holds central myMPD state and configuration values.
 */
struct t_mympd_state {
    //static config
    struct t_config *config;
    //mpd state
    struct t_mpd_shared_state *mpd_shared_state;
    //partition state
    struct t_partition_state *partition_state;
    //lists
    struct t_timer_list timer_list;
    struct t_list home_list;
    struct t_list trigger_list;
    //tags to use
    sds tag_list_search;
    sds tag_list_browse;
    //smart playlists
    bool smartpls;
    sds smartpls_sort;
    sds smartpls_prefix;
    time_t smartpls_interval;
    struct t_tags smartpls_generate_tag_types;
    sds smartpls_generate_tag_list;
    //columns
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
    //further configuration settings
    unsigned mpd_stream_port;
    sds music_directory;
    sds playlist_directory;
    sds navbar_icons;
    sds coverimage_names;
    sds thumbnail_names;
    unsigned volume_min;
    unsigned volume_max;
    unsigned volume_step;
    struct t_lyrics lyrics;
    sds listenbrainz_token;
    //settings only relevant for webui
    //saved as string containing json
    sds webui_settings;
};

/**
 * Public functions
 */
void mympd_state_save(struct t_mympd_state *mympd_state);

void mympd_state_default(struct t_mympd_state *mympd_state);
void mympd_state_free(struct t_mympd_state *mympd_state);

void mpd_shared_state_default(struct t_mpd_shared_state *mpd_shared_state);
void mpd_shared_state_features_disable(struct t_mpd_shared_state *mpd_shared_state);
void mpd_shared_state_free(struct t_mpd_shared_state *mpd_shared_state);

void partition_state_default(struct t_partition_state *partition_state, const char *name);
void partition_state_free(struct t_partition_state *partition_state);

void copy_tag_types(struct t_tags *src_tag_list, struct t_tags *dst_tag_list);
void reset_t_tags(struct t_tags *tags);

#endif
