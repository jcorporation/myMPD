/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2024 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#ifndef MYMPD_STATE_H
#define MYMPD_STATE_H

#include "dist/libmympdclient/include/mpd/client.h"
#include "dist/sds/sds.h"
#include "src/lib/cache.h"
#include "src/lib/config_def.h"
#include "src/lib/event.h"
#include "src/lib/list.h"
#include "src/lib/tags.h"

#include <time.h>

/**
 * Jukebox state
 */
enum jukebox_modes {
    JUKEBOX_OFF,        //!< jukebox is disabled
    JUKEBOX_ADD_SONG,   //!< jukebox adds single songs
    JUKEBOX_ADD_ALBUM,  //!< jukebox adds whole albums
    JUKEBOX_UNKNOWN     //!< jukebox mode is unknown
};

/**
 * MPD connection states
 */
enum mpd_conn_states {
    MPD_CONNECTED,     //!< mpd is connected
    MPD_DISCONNECTED,  //!< mpd is disconnected
    MPD_FAILURE        //!< mpd is in unrecoverable failure state
};

/**
 * MPD feature flags
 */
struct t_mpd_features {
    bool advqueue;                 //!< mpd supports the prio filter / sort for queue and the save modes
    bool albumart;                 //!< mpd supports the albumart command
    bool binarylimit;              //!< mpd supports the binarylimit command
    bool fingerprint;              //!< mpd supports the fingerprint command
    bool library;                  //!< myMPD has access to the mpd music directory
    bool mount;                    //!< mpd supports mounts
    bool neighbor;                 //!< mpd supports neighbors command
    bool partitions;               //!< mpd supports partitions
    bool playlists;                //!< mpd supports playlists
    bool playlist_rm_range;        //!< mpd supports the playlist rm range command
    bool readpicture;              //!< mpd supports the readpicture command
    bool stickers;                 //!< mpd supports stickers
    bool tags;                     //!< mpd tags are enabled
    bool whence;                   //!< mpd supports the whence feature (relative position in queue)
    bool consume_oneshot;          //!< mpd supports consume oneshot mode
    bool playlist_dir_auto;        //!< mpd supports autodetection of playlist directory
    bool starts_with;              //!< mpd supports starts_with filter expression
    bool pcre;                     //!< mpd supports pcre for filter expressions
    bool db_added;                 //!< mpd supports added attribute for songs
    bool sticker_sort_window;      //!< mpd supports sticker sort and window api
    bool sticker_int;              //!< mpd supports sticker value handling as integer
    bool search_add_sort_window;   //!< mpd supports search and window for findadd/searchadd/searchaddpl
    bool listplaylist_range;       //!< mpd supports the listplaylist with range parameter
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
    sds playlist_directory_value;       //!< real playlist directory set by feature detection
    //tags
    sds tag_list;                       //!< comma separated string of mpd tags to enable
    struct t_tags tags_mympd;           //!< tags enabled by myMPD and mpd
    struct t_tags tags_mpd;             //!< all available mpd tags
    struct t_tags tags_search;          //!< tags enabled for search
    struct t_tags tags_browse;          //!< tags enabled for browse
    struct t_tags tags_album;           //!< tags enabled for albums
    enum mpd_tag_type tag_albumartist;  //!< tag to use for AlbumArtist
    //Feature flags
    const unsigned *protocol;           //!< mpd protocol version
    struct t_mpd_features feat;         //!< feature flags
};

/**
 * Holds the jukebox states for a partition
 */
struct t_jukebox_state {
    enum jukebox_modes mode;       //!< the jukebox mode
    sds playlist;                  //!< playlist from which the jukebox queue is generated
    unsigned queue_length;         //!< how many songs should the mpd queue have
    unsigned last_played;          //!< only add songs with last_played state older than seconds from now
    struct t_tags uniq_tag;        //!< single tag for the jukebox uniq constraint
    struct t_list *queue;          //!< the jukebox queue itself
    bool ignore_hated;             //!< ignores hated songs for the jukebox mode
    sds filter_include;            //!< mpd search filter to include songs / albums
    sds filter_exclude;            //!< mpd search filter to exclude songs / albums
    unsigned min_song_duration;    //!< minimum song duration
    unsigned max_song_duration;    //!< maximum song duration
    bool filling;                  //!< indication flag for filling jukebox thread
    sds last_error;                //!< last jukebox error message
};

/**
 * Holds partition specific states
 */
struct t_partition_state {
    struct t_config *config;               //!< pointer to static config
    struct t_mpd_state *mpd_state;         //!< pointer to shared mpd state
    //mpd connection
    struct mpd_connection *conn;           //!< mpd connection object from libmpdclient
    enum mpd_conn_states conn_state;       //!< mpd connection state
    //track player states
    enum mpd_state play_state;             //!< mpd player state
    int song_id;                           //!< current song id from queue
    int next_song_id;                      //!< next song id from queue
    int last_song_id;                      //!< previous song id from queue
    int song_pos;                          //!< current song pos in queue
    sds song_uri;                          //!< current song uri
    sds last_song_uri;                     //!< previous song uri
    unsigned queue_version;                //!< queue version number (increments on queue change)
    unsigned queue_length;                 //!< length of the queue
    int last_skipped_id;                   //!< last skipped event was fired for this song id
    time_t song_end_time;                  //!< timestamp at which current song should end (starttime + duration)
    time_t last_song_end_time;             //!< timestamp at which previous song should end (starttime + duration)
    time_t song_start_time;                //!< timestamp at which current song has started
    time_t last_song_start_time;           //!< timestamp at which previous song has started
    time_t crossfade;                      //!< used for determine when to add next song from jukebox queue
    bool auto_play;                        //!< start play if queue changes
    bool player_error;                     //!< signals mpd player error condition
    struct t_jukebox_state jukebox;        //!< jukebox
    //partition
    sds name;                              //!< partition name
    sds highlight_color;                   //!< highlight color
    sds highlight_color_contrast;          //!< highlight contrast color
    sds state_dir;                         //!< partition state folder
    struct t_partition_state *next;        //!< pointer to next partition;
    bool is_default;                       //!< flag for the mpd default partition
    enum mpd_idle idle_mask;               //!< mpd idle mask
    bool set_conn_options;                 //!< true if mpd connection options should be changed
    //local playback
    unsigned mpd_stream_port;              //!< mpd http stream port setting
    sds stream_uri;                        //!< custom url for local playback
    //lists
    struct t_list last_played;             //!< last_played list
    struct t_list preset_list;             //!< Playback presets
    //timers
    int timer_fd_jukebox;                  //!< Timerfd for jukebox runs
    int timer_fd_scrobble;                 //!< Timerfd for scrobble event
    int timer_fd_mpd_connect;              //!< Timerfd for mpd reconnection
    //events
    enum pfd_type waiting_events;          //!< Bitmask for events
};

/**
 * Holds stickerdb specific states
 */
struct t_stickerdb_state {
    struct t_config *config;               //!< pointer to static config
    struct t_mpd_state *mpd_state;         //!< pointer to shared mpd state
    //mpd connection
    struct mpd_connection *conn;           //!< mpd connection object from libmpdclient
    enum mpd_conn_states conn_state;       //!< mpd connection state
    sds name;                              //!< name for logging
};

/**
 * Optional timer definition from GUI
 */
struct t_timer_definition {
    sds name;                         //!< name of the timer
    sds partition;                    //!< mpd partition
    bool enabled;                     //!< enabled flag
    int start_hour;                   //!< start hour
    int start_minute;                 //!< start minute
    sds action;                       //!< timer action, e.g. script, play
    sds subaction;                    //!< timer subaction, e.g. script to execute
    unsigned volume;                  //!< volume to set
    sds playlist;                     //!< playlist to load for play timer
    sds preset;                       //!< preset to load for play timer
    bool weekdays[7];                 //!< array of weekdays for timer execution
    struct t_list arguments;          //!< argumentlist for script timers
};

/**
 * Struct for timers containing a t_list with t_timer_nodes
 */
struct t_timer_list {
    unsigned last_id;                   //!< highest timer id in the list
    int active;                         //!< number of enabled timers
    struct t_list list;                 //!< timer definition
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
    struct t_mpd_state *mpd_state;                //!< mpd state shared across partitions
    struct t_partition_state *partition_state;    //!< list of partition states
    struct t_stickerdb_state *stickerdb;          //!< states for stickerdb connection
    struct mympd_pfds pfds;                       //!< fds to poll in the event loop
    struct t_timer_list timer_list;               //!< list of timers
    struct t_list home_list;                      //!< list of home icons
    struct t_list trigger_list;                   //!< list of triggers
    sds tag_list_search;                          //!< comma separated string of tags for search
    sds tag_list_browse;                          //!< comma separated string of tags for browse
    bool smartpls;                                //!< enable smart playlists
    sds smartpls_sort;                            //!< sort smart playlists by this tag
    sds smartpls_prefix;                          //!< name prefix for smart playlists
    int smartpls_interval;                        //!< interval to refresh smart playlists in seconds
    struct t_tags smartpls_generate_tag_types;    //!< generate smart playlists for each value for this tag
    sds smartpls_generate_tag_list;               //!< generate smart playlists for each value for this tag (string representation)
    sds cols_queue_current;                       //!< columns for the queue view
    sds cols_search;                              //!< columns for the search view
    sds cols_browse_database_album_detail_info;   //!< columns for the album detail view
    sds cols_browse_database_album_detail;        //!< columns for the album detail title list
    sds cols_browse_database_album_list;          //!< columns for the album list view
    sds cols_browse_playlist_list;                //!< columns for the listing of playlists
    sds cols_browse_playlist_detail;              //!< columns for the listing of playlist contents
    sds cols_browse_filesystem;                   //!< columns for filesystem listing
    sds cols_playback;                            //!< columns for playback view
    sds cols_queue_last_played;                   //!< columns for last played view
    sds cols_queue_jukebox_song;                  //!< columns for the jukebox queue view for songs
    sds cols_queue_jukebox_album;                 //!< columns for the jukebox queue view for albums
    sds cols_browse_radio_webradiodb;             //!< columns for the webradiodb view
    sds cols_browse_radio_radiobrowser;           //!< columns for the radiobrowser view
    sds music_directory;                          //!< mpd music directory setting (real value is in mpd_state)
    sds playlist_directory;                       //!< mpd playlist directory (real value is in mpd_state)
    sds navbar_icons;                             //!< json string of navigation bar icons
    sds coverimage_names;                         //!< comma separated string of coverimage names
    sds thumbnail_names;                          //!< comma separated string of coverimage thumbnail names
    unsigned volume_min;                          //!< minimum mpd volume
    unsigned volume_max;                          //!< maximum mpd volume
    unsigned volume_step;                         //!< volume step for +/- buttons
    struct t_lyrics lyrics;                       //!< lyrics settings
    sds listenbrainz_token;                       //!< listenbrainz token
    sds webui_settings;                           //!< settings only relevant for webui, saved as string containing json
    bool tag_disc_empty_is_first;                 //!< handle empty disc tag as disc one for albums
    sds booklet_name;                             //!< name of the booklet files
    sds info_txt_name;                            //!< name of album info files
    struct t_cache album_cache;                   //!< the album cache created by the mpd_worker thread
    unsigned last_played_count;                   //!< number of songs to keep in the last played list (disk + memory)
};

/**
 * Public functions
 */
void mympd_state_save(struct t_mympd_state *mympd_state, bool free_data);

void mympd_state_default(struct t_mympd_state *mympd_state, struct t_config *config);
void mympd_state_free(struct t_mympd_state *mympd_state);

void mpd_state_features_default(struct t_mpd_features *feat);
void mpd_state_features_copy(struct t_mpd_features *src, struct t_mpd_features *dst);

void mpd_state_default(struct t_mpd_state *mpd_state, struct t_config *config);
void mpd_state_copy(struct t_mpd_state *src, struct t_mpd_state *dst);
void mpd_state_free(struct t_mpd_state *mpd_state);

void partition_state_default(struct t_partition_state *partition_state, const char *name,
        struct t_mpd_state *mpd_state, struct t_config *config);
void partition_state_free(struct t_partition_state *partition_state);

void stickerdb_state_default(struct t_stickerdb_state *stickerdb, struct t_config *config);
void stickerdb_state_free(struct t_stickerdb_state *stickerdb);

void jukebox_state_default(struct t_jukebox_state *jukebox_state);
void jukebox_state_copy(struct t_jukebox_state *src, struct t_jukebox_state *dst);
void jukebox_state_free(struct t_jukebox_state *jukebox_state);

#endif
