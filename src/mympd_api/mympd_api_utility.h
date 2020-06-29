/*
 SPDX-License-Identifier: GPL-2.0-or-later
 myMPD (c) 2018-2020 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#ifndef __MYMPD_API_UTILITY_H
#define __MYMPD_API_UTILITY_H
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
    struct list arguments;
};

typedef void (*time_handler)(struct t_timer_definition *definition, void *user_data);

struct t_timer_node {
    int fd;
    time_handler callback;
    struct t_timer_definition *definition;
    void *user_data;
    unsigned int timeout;
    unsigned int interval;
    int timer_id;
    struct t_timer_node *next;
};

struct t_timer_list {
    int length;
    int last_id;
    int active;
    struct t_timer_node *list;
};

typedef struct t_mympd_state {
    sds mpd_host;
    int mpd_port;
    sds mpd_pass;
    bool stickers;
    sds taglist;
    sds searchtaglist;
    sds browsetaglist;
    bool smartpls;
    sds smartpls_sort;
    sds smartpls_prefix;
    time_t smartpls_interval;
    sds generate_pls_tags;
    int max_elements_per_page;
    int last_played_count;
    bool love;
    sds love_channel;
    sds love_message;
    bool notification_web;
    bool notification_page;
    bool media_session;
    bool auto_play;
    enum jukebox_modes jukebox_mode;
    sds jukebox_playlist;
    int jukebox_queue_length;
    int jukebox_last_played;
    sds jukebox_unique_tag;
    sds cols_queue_current;
    sds cols_search;
    sds cols_browse_database;
    sds cols_browse_playlists_detail;
    sds cols_browse_filesystem;
    sds cols_playback;
    sds cols_queue_last_played;
    bool localplayer;
    bool localplayer_autoplay;
    int stream_port;
    sds stream_url;
    bool bg_cover;
    sds bg_color;
    sds bg_css_filter;
    bool coverimage;
    sds coverimage_name;
    int coverimage_size;
    int covergrid_size;
    sds locale;
    sds music_directory;
    sds theme;
    sds highlight_color;
    bool bookmarks;
    bool timer;
    sds booklet_name;
    struct t_timer_list timer_list;
    bool lyrics;
} t_mympd_state;

void free_mympd_state(t_mympd_state *mympd_state);
void free_mympd_state_sds(t_mympd_state *mympd_state);
void mympd_api_push_to_mpd_client(t_mympd_state *mympd_state);
sds json_to_cols(sds cols, char *str, size_t len, bool *error);
#endif
