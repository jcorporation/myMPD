/*
 SPDX-License-Identifier: GPL-2.0-or-later
 myMPD (c) 2018-2020 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#ifndef __MPD_CLIENT_UTILITY_H__
#define __MPD_CLIENT_UTILITY_H__

#include "dist/src/rax/rax.h"

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
    bool feat_tags;
    bool feat_library;
    bool feat_advsearch;
    bool feat_smartpls;
    bool feat_love;
    bool feat_coverimage;
    bool feat_fingerprint;
    bool feat_mpd_albumart;
    bool feat_mpd_readpicture;
    bool feat_single_oneshot;
    bool feat_mpd_searchwindow;
    //mympd states
    enum jukebox_modes jukebox_mode;
    sds jukebox_playlist;
    size_t jukebox_queue_length;
    struct list jukebox_queue;
    struct list jukebox_queue_tmp;
    t_tags jukebox_unique_tag;
    int jukebox_last_played;
    bool auto_play;
    bool coverimage;
    sds coverimage_name;
    bool love;
    sds love_channel;
    sds love_message;
    sds taglist;
    sds searchtaglist;
    sds browsetaglist;
    sds generate_pls_tags;
    bool stickers;
    bool smartpls;
    sds smartpls_sort;
    sds smartpls_prefix;
    time_t smartpls_interval;
    sds mpd_host;
    int mpd_port;
    sds mpd_pass;
    int last_played_count;
    int max_elements_per_page;
    sds music_directory;
    sds music_directory_value;
    //taglists
    t_tags mpd_tag_types;
    t_tags mympd_tag_types;
    t_tags search_tag_types;
    t_tags browse_tag_types;
    t_tags generate_pls_tag_types;
    //last played list
    struct list last_played;
    //sticker cache
    rax *sticker_cache;
    sds booklet_name;
} t_mpd_state;

typedef struct t_sticker {
    int playCount;
    int skipCount;
    int lastPlayed;
    int lastSkipped;
    int like;
} t_sticker;

void disable_all_mpd_tags(t_mpd_state *mpd_state);
void enable_all_mpd_tags(t_mpd_state *mpd_state);
void enable_mpd_tags(t_mpd_state *mpd_state, t_tags enable_tags);
sds put_song_tags(sds buffer, t_mpd_state *mpd_state, const t_tags *tagcols, const struct mpd_song *song);
sds put_empty_song_tags(sds buffer, t_mpd_state *mpd_state, const t_tags *tagcols, const char *uri);
bool check_error_and_recover2(t_mpd_state *mpd_state, sds *buffer, sds method, int request_id, bool notify);
sds check_error_and_recover(t_mpd_state *mpd_state, sds buffer, sds method, int request_id);
sds check_error_and_recover_notify(t_mpd_state *mpd_state, sds buffer);
sds respond_with_mpd_error_or_ok(t_mpd_state *mpd_state, sds buffer, sds method, int request_id);
char *mpd_client_get_tag(struct mpd_song const *song, const enum mpd_tag_type tag);
bool mpd_client_tag_exists(const enum mpd_tag_type tag_types[64], const size_t tag_types_len, const enum mpd_tag_type tag);
void json_to_tags(const char *str, int len, void *user_data);
void reset_t_tags(t_tags *tags);
void free_mpd_state(t_mpd_state *mpd_state);
void default_mpd_state(t_mpd_state *mpd_state);
bool is_smartpls(t_config *config, t_mpd_state *mpd_state, const char *plpath);
void detect_extra_files(t_mpd_state *mpd_state, const char *uri, bool *booklet, bool *lyrics, struct list *images);
sds put_extra_files(t_mpd_state *mpd_state, sds buffer, const char *uri);
#endif
