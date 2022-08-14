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
#include <poll.h>
#include <time.h>

/**
 * Jukebox state
 */
enum jukebox_modes {
    JUKEBOX_OFF,       //!< jukebox is disabled
    JUKEBOX_ADD_SONG,  //!< jukebox adds single songs
    JUKEBOX_ADD_ALBUM, //!< jukebox adss whole albums
    JUKEBOX_UNKNOWN    //!< jukebox mode is unknown
};

/**
 * MPD connection states
 */
enum mpd_conn_states {
    MPD_CONNECTED,           //!< mpd is connected
    MPD_DISCONNECTED,        //!< mpd is disconnected, try to reconnect
    MPD_FAILURE,             //!< mpd connection failed, disconnect mpd and reconnect after wait time
    MPD_DISCONNECT,          //!< disconnect mpd and reconnect after wait time
    MPD_DISCONNECT_INSTANT,  //!< disconnect mpd and reconnect as soon as possible
    MPD_WAIT                 //!< wating for reconnection
};

/**
 * Struct for a mpd tag lists
 * libmpdclient uses the same declaration
 */
struct t_tags {
    size_t len;                 //!< number of tags in the array
    enum mpd_tag_type tags[64]; //!< tags array
};

/**
 * Holds cache information
 */
struct t_cache {
    bool building;  //!< true if the mpd_worker thread is creating the cache
    rax *cache;     //!< pointer to the cache
};

/**
 * Holds MPD specific states shared across all partitions
 */
struct t_mpd_state {
    struct t_config *config;            //!< pointer to static config
    //connection configuration
    sds mpd_host;                       //!< mpd host configuration
    unsigned mpd_port;                  //!< mpd port configuration
    sds mpd_pass;                       //!< mpd password
    unsigned mpd_binarylimit;           //!< mpd binary limit to set
    unsigned mpd_timeout;               //!< mpd connection timeout
    bool mpd_keepalive;                 //!< mpd tcp keepalive flag
    sds music_directory_value;          //!< real music directory set by feature detection
    //tags
    sds tag_list;                       //!< comma separated string of mpd tags to enable
    struct t_tags tags_mympd;           //!< tags enabled by myMPD and mpd
    struct t_tags tags_mpd;             //!< all available mpd tags
    struct t_tags tags_search;          //!< tags enabled for search
    struct t_tags tags_browse;          //!< tags enabled for browse
    enum mpd_tag_type tag_albumartist;  //!< tag to use for AlbumArtist
    //Feature flags
    const unsigned *protocol;           //!< mpd protocol version
    bool feat_advqueue;                 //!< mpd supports the prio filter / sort for queue
    bool feat_albumart;                 //!< mpd supports the albumart command
    bool feat_binarylimit;              //!< mpd supports the binarylimit command
    bool feat_fingerprint;              //!< mpd supports the fingerprint command
    bool feat_library;                  //!< myMPD has access to the mpd music directory
    bool feat_mount;                    //!< mpd supports mounts
    bool feat_neighbor;                 //!< mpd supports neighbors command
    bool feat_partitions;               //!< mpd supports partitions
    bool feat_playlists;                //!< mpd supports playlists
    bool feat_playlist_rm_range;        //!< mpd supports the playlist rm range command
    bool feat_readpicture;              //!< mpd supports the readpicture command
    bool feat_stickers;                 //!< mpd supports stickers
    bool feat_tags;                     //!< mpd tags are enabled
    bool feat_whence;                   //!< mpd supports the whence feature (relative position in queue)
    //caches
    struct t_cache album_cache;         //!< the album cache created by the mpd_worker thread
    struct t_cache sticker_cache;       //!< the sticker cache created by the mpd_worker thread
    //lists
    struct t_list last_played;          //!< last_played list
    long last_played_count;             //!< number of songs to keep in the last played list (disk + memory)
    struct t_list sticker_queue;        //!< queue for stickers to set (cache if sticker cache is rebuilding) 
    sds booklet_name;                   //!< name of the booklet files
};

/**
 * Holds partition specific states
 */
struct t_partition_state {
    //mpd connection
    struct mpd_connection *conn;           //!< mpd connection object from libmpdclient
    enum mpd_conn_states conn_state;       //!< mpd connection state
    //reconnect timer
    time_t reconnect_time;                 //!< timestamp at which next connection attempt is made
    time_t reconnect_interval;             //!< interval for connections attempts (increases by failed attempts)
    //track player states
    enum mpd_state play_state;             //!< mpd player state
    int song_id;                           //!< current song id from queue
    int next_song_id;                      //!< next song id from queue
    int last_song_id;                      //!< previous song id from queue
    int song_pos;                          //!< current song pos in queue
    sds song_uri;                          //!< current song uri
    sds last_song_uri;                     //!< previous song uri
    unsigned queue_version;                //!< queue version number (increments on queue change)
    long long queue_length;                //!< length of the queue
    int last_last_played_id;               //!< last scrobble event was fired for this song id
    int last_skipped_id;                   //!< last skipped event was fired for this song id
    time_t song_end_time;                  //!< timestamp at which current song should end (starttime + duration)
    time_t last_song_end_time;             //!< timestamp at which previous song should end (starttime + duration)
    time_t song_start_time;                //!< timestamp at which current song has started
    time_t last_song_start_time;           //!< timestap at which previous song has started
    time_t crossfade;                      //!< used for determine when to add next song from jukebox queue
    time_t set_song_played_time;           //!< timestamp at which the next scrobble event will be fired
    time_t last_song_set_song_played_time; //!< timestamp of the previous scrobble event
    bool auto_play;                        //!< start play if queue changes
    //jukebox
    enum jukebox_modes jukebox_mode;       //!< the jukebox mode
    sds jukebox_playlist;                  //!< playlist from which the jukebox queue is generated
    long jukebox_queue_length;             //!< how many songs should the mpd queue have
    long jukebox_last_played;              //!< only add songs with last_played state older than this timestamp
    struct t_tags jukebox_unique_tag;      //!< single tag for the jukebox unique constraint
    bool jukebox_enforce_unique;           //!< flag indicating if unique constraint is enabled
    struct t_list jukebox_queue;           //!< the jukebox queue itself
    struct t_list jukebox_queue_tmp;       //!< temporaray jukebox queue for the add random to queue function
    struct t_mpd_state *mpd_state;         //!< pointer to shared MPD state
    //partition
    sds name;                              //!< partition name
    struct t_partition_state *next;        //!< pointer to next partition;
    bool is_default;                       //!< flag for the mpd default partition
};

/**
 * Optional timer definition from GUI
 */
struct t_timer_definition {
    sds name;                         //!< name of the timer
    bool enabled;                     //!< enabled flag
    int start_hour;                   //!< start hour
    int start_minute;                 //!< start minute
    sds action;                       //!< timer action, e.g. script, play
    sds subaction;                    //!< timer subaction, e.g. script to execute
    unsigned volume;                  //!< volume to set
    sds playlist;                     //!< playlist to use
    enum jukebox_modes jukebox_mode;  //!< jukebox mode
    bool weekdays[7];                 //!< array of weekdays for timer execution
    struct t_list arguments;          //!< argumentlist for script timers
};

/**
 * forward declaration
 */
struct t_timer_node;

/**
 * Linked list of timers containing t_timer_nodes
 */
struct t_timer_list {
    int length;                 //!< length of the timer list
    int last_id;                //!< highest timer id in the list
    int active;                 //!< number of enabled timers
    struct t_timer_node *list;  //!< timer definition
};

/**
 * Lyrics settings
 */
struct t_lyrics {
    sds uslt_ext;     //!< fileextension for unsynced lyrics
    sds sylt_ext;     //!< fileextension for synced lyrics
    sds vorbis_uslt;  //!< vorbis comment for unsynced lyrics
    sds vorbis_sylt;  //!< vorbis comment for synced lyrics
};

/**
 * Holds central myMPD state and configuration values.
 */
struct t_mympd_state {
    struct t_config *config;                      //!< pointer to static config
    struct t_mpd_state *mpd_state;                //!< mpd state shared accross partitions
    struct t_partition_state *partition_state;    //!< list of partition states
    struct pollfd fds[MPD_CONNECTION_MAX];        //!< mpd connection fds
    nfds_t nfds;                                     //!< number of mpd connection fds
    struct t_timer_list timer_list;               //!< list of timers
    struct t_list home_list;                      //!< list of home icons
    struct t_list trigger_list;                   //!< list of triggers
    sds tag_list_search;                          //!< comma separated string of tags for search
    sds tag_list_browse;                          //!< comma separated string of tags for browse
    bool smartpls;                                //!< enable smart playlists
    sds smartpls_sort;                            //!< sort smart playlists by this tag
    sds smartpls_prefix;                          //!< name prefix for smart playlists
    time_t smartpls_interval;                     //!< interval to refresh smart playlists in seconds
    struct t_tags smartpls_generate_tag_types;    //!< generate smart playlists for each value for this tag
    sds smartpls_generate_tag_list;               //!< generate smart playlists for each value for this tag (string representation)
    sds cols_queue_current;                       //!< columns for the queue view
    sds cols_search;                              //!< columns for the search view
    sds cols_browse_database_detail;              //!< columns for the album detail view
    sds cols_browse_playlists_detail;             //!< columns for the listing of playlists
    sds cols_browse_filesystem;                   //!< columns for filesystem listing
    sds cols_playback;                            //!< columns for plaback view
    sds cols_queue_last_played;                   //!< columns for last played view
    sds cols_queue_jukebox;                       //!< columns for the jukebox queue view
    sds cols_browse_radio_webradiodb;             //!< columns for the webradiodb view
    sds cols_browse_radio_radiobrowser;           //!< columns for the radiobrowser view
    unsigned mpd_stream_port;                     //!< mpd http stream port setting
    sds music_directory;                          //!< mpd music directory setting (real value is in mpd_state)
    sds playlist_directory;                       //!< mpd playlist directory
    sds navbar_icons;                             //!< json strin of navigation bar icons
    sds coverimage_names;                         //!< comma separated string of coverimage names
    sds thumbnail_names;                          //!< comma separated string of coverimage thumbnail names
    unsigned volume_min;                          //!< minimum mpd volume
    unsigned volume_max;                          //!< maximum mpd volume
    unsigned volume_step;                         //!< volume step for +/- buttons
    struct t_lyrics lyrics;                       //!< lyrics settings
    sds listenbrainz_token;                       //!< listenbrainz token
    sds webui_settings;                           //!< settings only relevant for webui, saved as string containing json
};

/**
 * Public functions
 */
void mympd_state_save(struct t_mympd_state *mympd_state);

void mympd_state_default(struct t_mympd_state *mympd_state);
void mympd_state_free(struct t_mympd_state *mympd_state);

void mpd_state_default(struct t_mpd_state *mpd_state);
void mpd_state_features_disable(struct t_mpd_state *mpd_state);
void mpd_state_free(struct t_mpd_state *mpd_state);

void partition_state_default(struct t_partition_state *partition_state, const char *name);
void partition_state_free(struct t_partition_state *partition_state);

void copy_tag_types(struct t_tags *src_tag_list, struct t_tags *dst_tag_list);
void reset_t_tags(struct t_tags *tags);

#endif
