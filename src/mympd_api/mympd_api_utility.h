/* myMPD
   (c) 2018-2019 Juergen Mang <mail@jcgames.de>
   This project's homepage is: https://github.com/jcorporation/mympd
   
   myMPD ist fork of:
   
   ympd
   (c) 2013-2014 Andrew Karpow <andy@ndyk.de>
   This project's homepage is: http://www.ympd.org
   
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; version 2 of the License.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License along
   with this program; if not, write to the Free Software Foundation, Inc.,
   Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
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
#endif
