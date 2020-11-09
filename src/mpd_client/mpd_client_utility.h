/*
 SPDX-License-Identifier: GPL-2.0-or-later
 myMPD (c) 2018-2020 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#ifndef __MPD_CLIENT_UTILITY_H__
#define __MPD_CLIENT_UTILITY_H__

#include "../dist/src/rax/rax.h"

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

typedef struct t_mpd_client_state {
    // States
    int song_id;
    int next_song_id;
    int last_song_id;
    sds song_uri;
    sds last_song_uri;
    unsigned queue_version;
    unsigned queue_length;
    int last_last_played_id;
    int last_skipped_id;
    time_t song_end_time;
    time_t last_song_end_time;
    time_t song_start_time;
    time_t last_song_start_time;
    time_t crossfade;
    time_t set_song_played_time;
    // Features
    const unsigned* protocol;
    bool feat_sticker;
    bool feat_playlists;
    bool feat_library;
    bool feat_smartpls;
    bool feat_love;
    bool feat_coverimage;
    bool feat_fingerprint;
    bool feat_mpd_albumart;
    bool feat_mpd_readpicture;
    bool feat_single_oneshot;
    bool feat_mpd_mount;
    bool feat_mpd_neighbor;
    bool feat_mpd_partitions;
    //mympd states
    enum jukebox_modes jukebox_mode;
    sds jukebox_playlist;
    size_t jukebox_queue_length;
    struct list jukebox_queue;
    struct list jukebox_queue_tmp;
    t_tags jukebox_unique_tag;
    int jukebox_last_played;
    bool jukebox_enforce_unique;
    bool auto_play;
    bool coverimage;
    sds coverimage_name;
    bool love;
    sds love_channel;
    sds love_message;
    sds searchtaglist;
    sds browsetaglist;
    sds generate_pls_tags;
    bool stickers;
    bool smartpls;
    sds smartpls_sort;
    sds smartpls_prefix;
    time_t smartpls_interval;
    unsigned last_played_count;
    int max_elements_per_page;
    sds music_directory;
    sds music_directory_value;
    sds booklet_name;
    //taglists
    t_tags search_tag_types;
    t_tags browse_tag_types;
    t_tags generate_pls_tag_types;
    //last played list
    struct list last_played;
    //sticker cache
    rax *sticker_cache;
    struct list sticker_queue;
    bool sticker_cache_building;
    //mpd state
    struct t_mpd_state *mpd_state;
    //triggers
    struct list triggers;
} t_mpd_client_state;

void json_to_tags(const char *str, int len, void *user_data);
void free_mpd_client_state(t_mpd_client_state *mpd_client_state);
void default_mpd_client_state(t_mpd_client_state *mpd_client_state);
bool is_smartpls(t_config *config, t_mpd_client_state *mpd_client_state, const char *plpath);
sds put_extra_files(t_mpd_client_state *mpd_client_state, sds buffer, const char *uri, bool is_dirname);
#endif
