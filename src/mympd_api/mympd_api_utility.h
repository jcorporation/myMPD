/*
 SPDX-License-Identifier: GPL-2.0-or-later
 myMPD (c) 2018-2019 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#ifndef __MYMPD_API_UTILITY_H
#define __MYMPD_API_UTILITY_H
typedef struct t_mympd_state {
    sds mpd_host;
    int mpd_port;
    sds mpd_pass;
    bool stickers;
    sds taglist;
    sds searchtaglist;
    sds browsetaglist;
    bool smartpls;
    int max_elements_per_page;
    int last_played_count;
    bool love;
    sds love_channel;
    sds love_message;
    bool notification_web;
    bool notification_page;
    bool auto_play;
    enum jukebox_modes jukebox_mode;
    sds jukebox_playlist;
    int jukebox_queue_length;
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
    sds locale;
    sds music_directory;
} t_mympd_state;

void free_mympd_state(t_mympd_state *mympd_state);
void mympd_api_push_to_mpd_client(t_mympd_state *mympd_state);
void json_to_cols(const char *str, int len, void *user_data);
#endif
