/* myMPD (c) 2018-2019 Juergen Mang <mail@jcgames.de> This project's
   homepage is: https://github.com/jcorporation/mympd
   
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

#ifndef __MPD_CLIENT_UTILS_H__
#define __MPD_CLIENT_UTILS_H__

#define RETURN_ERROR_AND_RECOVER(X) do { \
    LOG_ERROR("MPD error %s: %s", X, mpd_connection_get_error_message(mpd_state->conn)); \
    buffer = sdscat(sdsempty(), "{\"type\":\"error\","); \
    buffer = tojson_char(buffer, "data", mpd_connection_get_error_message(mpd_state->conn), false); \
    buffer = sdscat(buffer, "}"); \
    if (!mpd_connection_clear_error(mpd_state->conn)) { \
        mpd_state->conn_state = MPD_FAILURE; \
    } \
    return buffer; \
} while (0)

#define LOG_ERROR_AND_RECOVER(X) do { \
    LOG_ERROR("MPD error %s: %s", X, mpd_connection_get_error_message(mpd_state->conn)); \
    if (!mpd_connection_clear_error(mpd_state->conn)) { \
        mpd_state->conn_state = MPD_FAILURE; \
    } \
} while (0)

#define PUT_SONG_TAG_COLS(TAGCOLS) do { \
    if (mpd_state->feat_tags == true) { \
        for (int tagnr = 0; tagnr < TAGCOLS->len; ++tagnr) { \
            if (tagnr > 0) \
                buffer = sdscat(buffer, ","); \
            char *tag_value = mpd_client_get_tag(song, TAGCOLS->tags[tagnr]); \
            buffer = tojson_char(buffer, mpd_tag_name(TAGCOLS->tags[tagnr]), tag_value == NULL ? "-" : tag_value, false); \
        } \
    } \
    else { \
        char *tag_value = mpd_client_get_tag(song, MPD_TAG_TITLE); \
        buffer = tojson_char(buffer, "Title", tag_value == NULL ? "-" : tag_value, false); \
    } \
    buffer = sdscat(buffer, ","); \
    buffer = tojson_long(buffer, "Duration", mpd_song_get_duration(song), true); \
    buffer = tojson_long(buffer, "uri", mpd_song_get_uri(song), false); \
} while (0)

#define PUT_SONG_TAG_ALL() do { \
    if (mpd_state->feat_tags == true) { \
        for (unsigned tagnr = 0; tagnr < mpd_state->mympd_tag_types_len; ++tagnr) { \
            if (tagnr > 0) \
                len += json_printf(&out, ","); \
            char *tag_value = mpd_client_get_tag(song, mpd_state->mympd_tag_types[tagnr]); \
            buffer = tojson_char(buffer, mpd_tag_name(mpd_state->mympd_tag_types[tagnr]), tag_value == NULL ? "-" : tag_value, false); \
        } \
    } \
    else { \
        char *tag_value = mpd_client_get_tag(song, MPD_TAG_TITLE); \
        buffer = tojson_char(buffer, "Title", tag_value == NULL ? "-" : tag_value, false); \
    } \
    buffer = sdscat(buffer, ","); \
    buffer = tojson_long(buffer, "Duration", mpd_song_get_duration(song), true); \
    buffer = tojson_long(buffer, "uri", mpd_song_get_uri(song), false); \
} while (0)

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
    unsigned reconnect_intervall;
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
    //mympd states
    enum jukebox_modes jukebox_mode;
    sds jukebox_playlist;
    size_t jukebox_queue_length;
    bool auto_play;
    bool coverimage;
    sds coverimage_name;
    bool love;
    sds love_channel;
    sds love_message;
    sds taglist;
    sds searchtaglist;
    sds browsetaglist;
    bool stickers;
    bool smartpls;
    sds mpd_host;
    int mpd_port;
    sds mpd_pass;
    int last_played_count;
    int max_elements_per_page;
    sds music_directory;
    sds music_directory_value;
    //taglists
    enum mpd_tag_type mpd_tag_types[64];
    size_t mpd_tag_types_len;
    enum mpd_tag_type mympd_tag_types[64];
    size_t mympd_tag_types_len;
    enum mpd_tag_type search_tag_types[64];
    size_t search_tag_types_len;
    enum mpd_tag_type browse_tag_types[64];
    size_t browse_tag_types_len;
    //last played list
    struct list last_played;
} t_mpd_state;

char *mpd_client_get_tag(struct mpd_song const *song, const enum mpd_tag_type tag);
bool mpd_client_tag_exists(const enum mpd_tag_type tag_types[64], const size_t tag_types_len, const enum mpd_tag_type tag);
void json_to_tags(const char *str, int len, void *user_data);
#endif
