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

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <libgen.h>
#include <ctype.h>
#include <libgen.h>
#include <poll.h>
#include <dirent.h>
#include <pthread.h>
#include <mpd/client.h>
#include <signal.h>
#include <assert.h>
#include <inttypes.h>

#include "list.h"
#include "tiny_queue.h"
#include "global.h"
#include "mpd_client.h"
#include "../dist/src/frozen/frozen.h"

//private definitions
#define RETURN_ERROR_AND_RECOVER(X) do { \
    LOG_ERROR("MPD error %s: %s", X, mpd_connection_get_error_message(mpd_state->conn)); \
    len = json_printf(&out, "{type: error, data: %Q}", mpd_connection_get_error_message(mpd_state->conn)); \
    if (!mpd_connection_clear_error(mpd_state->conn)) \
        mpd_state->conn_state = MPD_FAILURE; \
    return len; \
} while (0)

#define LOG_ERROR_AND_RECOVER(X) do { \
    LOG_ERROR("MPD error %s: %s", X, mpd_connection_get_error_message(mpd_state->conn)); \
    if (!mpd_connection_clear_error(mpd_state->conn)) \
        mpd_state->conn_state = MPD_FAILURE; \
} while (0)

#define PUT_SONG_TAG_COLS(TAGCOLS) do { \
    if (mpd_state->feat_tags == true) { \
        for (unsigned tagnr = 0; tagnr < TAGCOLS->len; ++tagnr) { \
            if (tagnr > 0) \
                len += json_printf(&out, ","); \
            char *tag_value = mpd_client_get_tag(song, TAGCOLS->tags[tagnr]); \
            len += json_printf(&out, "%Q: %Q",  mpd_tag_name(TAGCOLS->tags[tagnr]), tag_value == NULL ? "-" : tag_value); \
        } \
    } \
    else { \
        char *tag_value = mpd_client_get_tag(song, MPD_TAG_TITLE); \
        len += json_printf(&out, "Title: %Q", tag_value == NULL ? "-" : tag_value); \
    } \
    len += json_printf(&out, ", Duration: %d, uri: %Q", mpd_song_get_duration(song), mpd_song_get_uri(song)); \
} while (0)

#define PUT_SONG_TAG_ALL() do { \
    if (mpd_state->feat_tags == true) { \
        for (unsigned tagnr = 0; tagnr < mpd_state->mympd_tag_types_len; ++tagnr) { \
            if (tagnr > 0) \
                len += json_printf(&out, ","); \
            char *value = mpd_client_get_tag(song, mpd_state->mympd_tag_types[tagnr]); \
            len += json_printf(&out, "%Q: %Q",  mpd_tag_name(mpd_state->mympd_tag_types[tagnr]), value == NULL ? "-" : value); \
        } \
    } \
    else { \
        char *value = mpd_client_get_tag(song, MPD_TAG_TITLE); \
        len += json_printf(&out, "Title: %Q", value == NULL ? "-" : value); \
    } \
    len += json_printf(&out, ", Duration: %d, uri: %Q", mpd_song_get_duration(song), mpd_song_get_uri(song)); \
} while (0)

enum mpd_conn_states {
    MPD_DISCONNECTED,
    MPD_FAILURE,
    MPD_CONNECTED,
    MPD_RECONNECT,
    MPD_DISCONNECT,
    MPD_WAIT
};

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
    char *song_uri;
    char *last_song_uri;
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
    char *jukebox_playlist;
    size_t jukebox_queue_length;
    bool auto_play;
    bool coverimage;
    char *coverimage_name;
    bool love;
    char *love_channel;
    char *love_message;
    char *taglist;
    char *searchtaglist;
    char *browsetaglist;
    bool stickers;
    bool smartpls;
    char *mpd_host;
    int mpd_port;
    char *mpd_pass;
    int last_played_count;
    int max_elements_per_page;
    char *music_directory;
    char *music_directory_value;
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

typedef struct t_sticker {
    int playCount;
    int skipCount;
    int lastPlayed;
    int lastSkipped;
    int like;
} t_sticker;

typedef struct t_tags {
    size_t len;
    enum mpd_tag_type tags[64];
} t_tags;

static void mpd_client_idle(t_config *config, t_mpd_state *mpd_state);
static void mpd_client_parse_idle(t_config *config, t_mpd_state *mpd_state, const int idle_bitmask);
static void mpd_client_api(t_config *config, t_mpd_state *mpd_state, void *arg_request);
static void mpd_client_notify(const char *message, const size_t n);
static bool mpd_client_count_song_uri(t_mpd_state *mpd_state, const char *uri, const char *name, const int value);
static bool mpd_client_like_song_uri(t_mpd_state *mpd_state, const char *uri, int value);
static bool mpd_client_last_played_song_uri(t_mpd_state *mpd_state, const char *uri);
static bool mpd_client_last_skipped_song_uri(t_mpd_state *mpd_state, const char *uri);
static bool mpd_client_get_sticker(t_mpd_state *mpd_state, const char *uri, t_sticker *sticker);
static bool mpd_client_last_played_list(t_config *config, t_mpd_state *mpd_state, const int song_id);
static bool mpd_client_last_played_list_save(t_config *config, t_mpd_state *mpd_state);
static bool mpd_client_jukebox(t_mpd_state *mpd_state);
static bool mpd_client_jukebox_add(t_mpd_state *mpd_state, const int addSongs, const enum jukebox_modes jukebox_mode, const char *jukebox_playlist);
static bool mpd_client_smartpls_save(t_config *config, t_mpd_state *mpd_state, const char *smartpltype, const char *playlist, const char *tag, const char *searchstr, const int maxentries, const int timerange);
static int mpd_client_smartpls_put(t_config *config, char *buffer, const char *playlist);
static bool mpd_client_smartpls_update_all(t_config *config, t_mpd_state *mpd_state);
static bool mpd_client_smartpls_update(t_config *config, t_mpd_state *mpd_state, char *playlist);
static bool mpd_client_smartpls_clear(t_mpd_state *mpd_state, const char *playlist);
static bool mpd_client_smartpls_update_sticker(t_mpd_state *mpd_state, const char *playlist, const char *sticker, const int maxentries);
static bool mpd_client_smartpls_update_newest(t_mpd_state *mpd_state, const char *playlist, const int timerange);
static bool mpd_client_smartpls_update_search(t_mpd_state *mpd_state, const char *playlist, const char *tag, const char *searchstr);
static int mpd_client_get_updatedb_state(t_mpd_state *mpd_state, char *buffer);
static int mpd_client_get_state(t_mpd_state *mpd_state, char *buffer);
static int mpd_client_put_state(t_mpd_state *mpd_state, struct mpd_status *status, char *buffer);
static int mpd_client_get_queue_state(t_mpd_state *mpd_state, char *buffer);
static int mpd_client_put_queue_state(struct mpd_status *status, char *buffer);
static int mpd_client_put_outputs(t_mpd_state *mpd_state, char *buffer);
static int mpd_client_put_current_song(t_config *config, t_mpd_state *mpd_state, char *buffer);
static int mpd_client_put_queue(t_mpd_state *mpd_state, char *buffer, const unsigned int offset, const t_tags *tagcols);
static int mpd_client_put_browse(t_config *config, t_mpd_state *mpd_state, char *buffer, const char *path, const unsigned int offset, const char *filter, const t_tags *tagcols);
static int mpd_client_search(t_mpd_state *mpd_state, char *buffer, const char *searchstr, const char *filter, const char *plist, const unsigned int offset, const t_tags *tagcols);
static int mpd_client_search_adv(t_mpd_state *mpd_state, char *buffer, const char *expression, const char *sort, const bool sortdesc, const char *grouptag, const char *plist, const unsigned int offset, const t_tags *tagcols);
static int mpd_client_search_queue(t_mpd_state *mpd_state, char *buffer, const char *mpdtagtype, const unsigned int offset, const char *searchstr, const t_tags *tagcols);
static int mpd_client_put_volume(t_mpd_state *mpd_state, char *buffer);
static int mpd_client_put_stats(t_mpd_state *mpd_state, char *buffer);
static int mpd_client_put_settings(t_mpd_state *mpd_state, char *buffer);
static int mpd_client_put_db_tag(t_mpd_state *mpd_state, char *buffer, const unsigned int offset, const char *mpdtagtype, const char *mpdsearchtagtype, const char *searchstr, const char *filter);
static int mpd_client_put_songs_in_album(t_config *config, t_mpd_state *mpd_state, char *buffer, const char *album, const char *search, const char *tag, const t_tags *tagcols);
static int mpd_client_rename_playlist(t_config *config, t_mpd_state *mpd_state, char *buffer, const char *old_playlist, const char *new_playlist);
static int mpd_client_put_playlists(t_config *config, t_mpd_state *mpd_state, char *buffer, const unsigned int offset, const char *filter);
static int mpd_client_put_playlist_list(t_config *config, t_mpd_state *mpd_state, char *buffer, const char *uri, const unsigned int offset, const char *filter, const t_tags *tagcols);
static int mpd_client_put_songdetails(t_config *config, t_mpd_state *mpd_state, char *buffer, const char *uri);
static int mpd_client_put_fingerprint(t_mpd_state *mpd_state, char *buffer, const char *uri);
static int mpd_client_put_last_played_songs(t_config *config, t_mpd_state *mpd_state, char *buffer, const unsigned int offset, const t_tags *tagcols);
static int mpd_client_queue_crop(t_mpd_state *mpd_state, char *buffer);
static void mpd_client_disconnect(t_mpd_state *mpd_state);
static void mpd_client_feature_love(t_mpd_state *mpd_state);
static void mpd_client_feature_tags(t_mpd_state *mpd_state);
static void mpd_client_feature_music_directory(t_mpd_state *mpd_state);
static bool mpd_client_tag_exists(const enum mpd_tag_type tag_types[64], const size_t tag_types_len, const enum mpd_tag_type tag);
static void mpd_client_mpd_features(t_mpd_state *mpd_state);
static char *mpd_client_get_tag(struct mpd_song const *song, const enum mpd_tag_type tag);
static void json_to_tags(const char *str, int len, void *user_data);

//public functions
void *mpd_client_loop(void *arg_config) {
    t_config *config = (t_config *) arg_config;
    //State of mpd connection
    t_mpd_state *mpd_state = (t_mpd_state *)malloc(sizeof(t_mpd_state));
    mpd_state->conn_state = MPD_DISCONNECTED;
    mpd_state->reconnect_time = 0;
    mpd_state->reconnect_intervall = 0;
    mpd_state->timeout = 10000;
    mpd_state->state = MPD_STATE_UNKNOWN;
    mpd_state->song_id = -1;
    mpd_state->next_song_id = -1;
    mpd_state->last_song_id = -1;
    mpd_state->song_uri = NULL;
    mpd_state->last_song_uri = NULL;
    mpd_state->queue_version = 0;
    mpd_state->queue_length = 0;
    mpd_state->last_last_played_id = -1;
    mpd_state->song_end_time = 0;
    mpd_state->song_start_time = 0;
    mpd_state->last_song_end_time = 0;
    mpd_state->last_song_start_time = 0;
    mpd_state->last_skipped_id = 0;
    mpd_state->crossfade = 0;
    mpd_state->set_song_played_time = 0;
    mpd_state->mpd_tag_types_len = 0;
    memset(mpd_state->mpd_tag_types, 0, sizeof(mpd_state->mpd_tag_types));
    mpd_state->mympd_tag_types_len = 0;
    memset(mpd_state->mympd_tag_types, 0, sizeof(mpd_state->mympd_tag_types));
    mpd_state->search_tag_types_len = 0;
    memset(mpd_state->search_tag_types, 0, sizeof(mpd_state->search_tag_types));
    mpd_state->browse_tag_types_len = 0;
    memset(mpd_state->browse_tag_types, 0, sizeof(mpd_state->browse_tag_types));
    mpd_state->music_directory = NULL;
    mpd_state->music_directory_value = NULL;
    mpd_state->jukebox_playlist = NULL;
    mpd_state->song_uri = NULL;
    mpd_state->last_song_uri = NULL;
    mpd_state->coverimage_name = NULL;
    mpd_state->love_channel = NULL;
    mpd_state->love_message = NULL;
    mpd_state->taglist = NULL;
    mpd_state->searchtaglist = NULL;
    mpd_state->browsetaglist = NULL;
    mpd_state->mpd_host = NULL;
    mpd_state->mpd_pass = NULL;

    //wait for initial settings
    while (s_signal_received == 0) {
        t_work_request *request = tiny_queue_shift(mpd_client_queue, 50);
        if (request != NULL) {
            if (request->cmd_id == MYMPD_API_SETTINGS_SET) {
                LOG_DEBUG("Got initial settings from mympd_api");
                mpd_client_api(config, mpd_state, request);
                break;
            }
            else {
               //create response struct
               if (request->conn_id > -1) {
                    t_work_result *response = (t_work_result*)malloc(sizeof(t_work_result));
                    assert(response);
                    response->conn_id = request->conn_id;
                    response->length = snprintf(response->data, MAX_SIZE, "{\"type\": \"error\", \"data\": \"MPD disconnected\"}");
                    LOG_DEBUG("Send http response to connection %lu (first 800 chars):\n%*.*s", request->conn_id, 0, 800, response->data);
                    tiny_queue_push(web_server_queue, response);
                }
                LOG_DEBUG("mpd_client not initialized, discarding message");
                FREE_PTR(request);
            }
        }
    }
    //init last played songs list
    list_init(&mpd_state->last_played);

    LOG_INFO("Starting mpd_client");
    //On startup connect instantly
    mpd_state->conn_state = MPD_DISCONNECTED;
    while (s_signal_received == 0) {
        mpd_client_idle(config, mpd_state);
    }
    //Cleanup
    mpd_client_disconnect(mpd_state);
    mpd_client_last_played_list_save(config, mpd_state);
    FREE_PTR(mpd_state->music_directory);
    FREE_PTR(mpd_state->music_directory_value);
    FREE_PTR(mpd_state->jukebox_playlist);
    FREE_PTR(mpd_state->song_uri);
    FREE_PTR(mpd_state->last_song_uri);
    FREE_PTR(mpd_state->coverimage_name);
    FREE_PTR(mpd_state->love_channel);
    FREE_PTR(mpd_state->love_message);
    FREE_PTR(mpd_state->taglist);
    FREE_PTR(mpd_state->searchtaglist);
    FREE_PTR(mpd_state->browsetaglist);
    FREE_PTR(mpd_state->mpd_host);
    FREE_PTR(mpd_state->mpd_pass);
    free(mpd_state);
    mpd_state = NULL;
    return NULL;
}

//private functions
static void mpd_client_api(t_config *config, t_mpd_state *mpd_state, void *arg_request) {
    t_work_request *request = (t_work_request*) arg_request;
    unsigned int uint_buf1, uint_buf2, uint_rc;
    int je, int_buf1, int_rc; 
    float float_buf;
    bool bool_buf, rc;
    char *p_charbuf1 = NULL;
    char *p_charbuf2 = NULL;
    char *p_charbuf3 = NULL;
    char *p_charbuf4 = NULL;

    LOG_VERBOSE("API request (%d): %.*s", request->conn_id, request->length, request->data);
    //create response struct
    t_work_result *response = (t_work_result*)malloc(sizeof(t_work_result));
    assert(response);
    response->conn_id = request->conn_id;
    response->length = 0;
    
    switch(request->cmd_id) {
        case MPD_API_LOVE:
            if (mpd_run_send_message(mpd_state->conn, mpd_state->love_channel, mpd_state->love_message)) {
                response->length = snprintf(response->data, MAX_SIZE, "{\"type\": \"result\", \"data\": \"Scrobbled love\"}");
            }
            else {
                response->length = snprintf(response->data, MAX_SIZE, "{\"type\": \"error\", \"data\": \"Failed to send love message to channel\"}");
            }
        break;
        case MPD_API_LIKE:
            if (mpd_state->feat_sticker) {
                je = json_scanf(request->data, request->length, "{data: {uri: %Q, like: %d}}", &p_charbuf1, &uint_buf1);
                if (je == 2) {        
                    if (!mpd_client_like_song_uri(mpd_state, p_charbuf1, uint_buf1)) {
                        response->length = snprintf(response->data, MAX_SIZE, "{\"type\": \"error\", \"data\": \"Failed to set like\"}");
                    }
                    else {
                        response->length = snprintf(response->data, MAX_SIZE, "{\"type\": \"result\", \"data\": \"ok\"}");
                    }
                    FREE_PTR(p_charbuf1);
                }
            } 
            else {
                response->length = snprintf(response->data, MAX_SIZE, "{\"type\": \"error\", \"data\": \"MPD stickers are disabled\"}");
                LOG_ERROR("MPD stickers are disabled");
            }
            break;
        case MPD_API_PLAYER_STATE:
            response->length = mpd_client_get_state(mpd_state, response->data);
            break;
        case MYMPD_API_SETTINGS_SET:
            //only update mpd_state, already saved and sanitized in mympd_api.c
            je = json_scanf(request->data, request->length, "{data: {jukeboxMode: %d}}", &int_buf1);
            if (je == 1) {
                mpd_state->jukebox_mode = int_buf1;
            }
            je = json_scanf(request->data, request->length, "{data: {jukeboxPlaylist: %Q}}", &p_charbuf1);
            if (je == 1) {
                REASSIGN_PTR(mpd_state->jukebox_playlist, p_charbuf1);
            }
            je = json_scanf(request->data, request->length, "{data: {jukeboxQueueLength: %d}}", &int_buf1);
            if (je == 1) {
                mpd_state->jukebox_queue_length = int_buf1;
            }
            je = json_scanf(request->data, request->length, "{data: {autoPlay: %B}}", &bool_buf);
            if (je == 1) {
                mpd_state->auto_play = bool_buf;
            }
            je = json_scanf(request->data, request->length, "{data: {coverimage: %B}}", &bool_buf);
            if (je == 1) {
                mpd_state->coverimage = bool_buf;
            }
            je = json_scanf(request->data, request->length, "{data: {coverimageName: %Q}}", &p_charbuf1);
            if (je == 1) {
                REASSIGN_PTR(mpd_state->coverimage_name, p_charbuf1);
            }
            je = json_scanf(request->data, request->length, "{data: {loveChannel: %Q}}", &p_charbuf1);
            if (je == 1)
                REASSIGN_PTR(mpd_state->love_channel, p_charbuf1);
            je = json_scanf(request->data, request->length, "{data: {loveMessage: %Q}}", &p_charbuf1);
            if (je == 1)
                REASSIGN_PTR(mpd_state->love_message, p_charbuf1);
            je = json_scanf(request->data, request->length, "{data: {love: %B}}", &bool_buf);
            if (je == 1) {
                mpd_state->love = bool_buf;
            }
            je = json_scanf(request->data, request->length, "{data: {taglist: %Q}}", &p_charbuf1);
            if (je == 1) {
                REASSIGN_PTR(mpd_state->taglist, p_charbuf1);
            }
            je = json_scanf(request->data, request->length, "{data: {searchtaglist: %Q}}", &p_charbuf1);
            if (je == 1) {
                REASSIGN_PTR(mpd_state->searchtaglist, p_charbuf1);
            }
            je = json_scanf(request->data, request->length, "{data: {browsetaglist: %Q}}", &p_charbuf1);
            if (je == 1) {
                REASSIGN_PTR(mpd_state->browsetaglist, p_charbuf1);
            }
            je = json_scanf(request->data, request->length, "{data: {stickers: %B}}", &bool_buf);
            if (je == 1) {
                mpd_state->stickers = bool_buf;
            }
            je = json_scanf(request->data, request->length, "{data: {smartpls: %B}}", &bool_buf);
            if (je == 1) {
                mpd_state->smartpls = bool_buf;
            }
            je = json_scanf(request->data, request->length, "{data: {musicDirectory: %Q}}", &p_charbuf1);
            if (je == 1) {
                REASSIGN_PTR(mpd_state->music_directory, p_charbuf1);
            }
            je = json_scanf(request->data, request->length, "{data: {mpdHost: %Q, mpdPort: %d, mpdPass: %Q}}", &p_charbuf1, &int_buf1, &p_charbuf2);
            if (je == 3) {
                if (mpd_state->mpd_host == NULL || mpd_state->mpd_pass == NULL) {
                    REASSIGN_PTR(mpd_state->mpd_host, p_charbuf1);
                    REASSIGN_PTR(mpd_state->mpd_pass, p_charbuf2);
                    mpd_state->mpd_port = int_buf1;
                }
                else if (strcmp(p_charbuf1, mpd_state->mpd_host) != 0 || (strcmp(p_charbuf2, "dontsetpassword") != 0 && strcmp(mpd_state->mpd_pass, p_charbuf2) != 0) || mpd_state->mpd_port != int_buf1) {
                    REASSIGN_PTR(mpd_state->mpd_host, p_charbuf1);
                    if (strcmp(p_charbuf2, "dontsetpassword") != 0) {
                        REASSIGN_PTR(mpd_state->mpd_pass, p_charbuf2);
                    }
                    else {
                        FREE_PTR(p_charbuf2);
                    }
                    mpd_state->mpd_port = int_buf1;
                    mpd_state->conn_state = MPD_DISCONNECT;
                }
                else {
                    FREE_PTR(p_charbuf1);
                    FREE_PTR(p_charbuf2);
                }
            }
            
            je = json_scanf(request->data, request->length, "{data: {lastPlayedCount: %d}}", &uint_buf1);
            if (je == 1) {
                mpd_state->last_played_count = uint_buf1;
            }
            je = json_scanf(request->data, request->length, "{data: {maxElementsPerPage: %d}}", &uint_buf1);
            if (je == 1) {
                mpd_state->max_elements_per_page = uint_buf1;
            }
            //feature detection
            if (mpd_state->conn_state == MPD_CONNECTED) {
                mpd_client_feature_love(mpd_state);
                mpd_client_feature_tags(mpd_state);
                mpd_client_feature_music_directory(mpd_state);
            }
            //set mpd options
            je = json_scanf(request->data, request->length, "{data: {random: %u}}", &uint_buf1);
            if (je == 1) {
                if (!mpd_run_random(mpd_state->conn, uint_buf1))
                    response->length = snprintf(response->data, MAX_SIZE, "{\"type\": \"error\", \"data\": \"Failed to set mpd state %%{state}\", \"values\": {\"state\": \"random\"}}");
            }
            je = json_scanf(request->data, request->length, "{data: {repeat: %u}}", &uint_buf1);
            if (je == 1) {
                if (!mpd_run_repeat(mpd_state->conn, uint_buf1))
                    response->length = snprintf(response->data, MAX_SIZE, "{\"type\": \"error\", \"data\": \"Failed to set mpd state %%{state}\", \"values\": {\"state\": \"repeat\"}}");
            }
            je = json_scanf(request->data, request->length, "{data: {consume: %u}}", &uint_buf1);
            if (je == 1) {
                if (!mpd_run_consume(mpd_state->conn, uint_buf1))
                    response->length = snprintf(response->data, MAX_SIZE, "{\"type\": \"error\", \"data\": \"Failed to set mpd state %%{state}\", \"values\": {\"state\": \"consume\"}}");
            }
            je = json_scanf(request->data, request->length, "{data: {single: %u}}", &uint_buf1);
            if (je == 1) {
                if (!mpd_run_single(mpd_state->conn, uint_buf1))
                    response->length = snprintf(response->data, MAX_SIZE, "{\"type\": \"error\", \"data\": \"Failed to set mpd state %%{state}\", \"values\": {\"state\": \"single\"}}");
            }
            je = json_scanf(request->data, request->length, "{data: {crossfade: %u}}", &uint_buf1);
            if (je == 1) {
                if (!mpd_run_crossfade(mpd_state->conn, uint_buf1))
                    response->length = snprintf(response->data, MAX_SIZE, "{\"type\": \"error\", \"data\": \"Failed to set mpd state %%{state}\", \"values\": {\"state\": \"crossfade\"}}");
            }
            if (config->mixramp == true) {
                je = json_scanf(request->data, request->length, "{data: {mixrampdb: %f}}", &float_buf);
                if (je == 1) {
                    if (!mpd_run_mixrampdb(mpd_state->conn, float_buf))
                        response->length = snprintf(response->data, MAX_SIZE, "{\"type\": \"error\", \"data\": \"Failed to set mpd state %%{state}\", \"values\": {\"state\": \"mixrampdb\"}}");
                }
                je = json_scanf(request->data, request->length, "{data: {mixrampdelay: %f}}", &float_buf);
                if (je == 1) {
                    if (!mpd_run_mixrampdelay(mpd_state->conn, float_buf))
                        response->length = snprintf(response->data, MAX_SIZE, "{\"type\": \"error\", \"data\": \"Failed to set mpd state %%{state}\", \"values\": {\"state\": \"mixrampdelay\"}}");
                }
            }
            
            je = json_scanf(request->data, request->length, "{data: {replaygain: %Q}}", &p_charbuf1);
            if (je == 1) {
                if (!mpd_send_command(mpd_state->conn, "replay_gain_mode", p_charbuf1, NULL)) {
                    response->length = snprintf(response->data, MAX_SIZE, "{\"type\": \"error\", \"data\": \"Failed to set mpd state %%{state}\", \"values\": {\"state\": \"replaygain\"}}");
                }
                mpd_response_finish(mpd_state->conn);
                FREE_PTR(p_charbuf1);            
            }
            if (mpd_state->jukebox_mode != JUKEBOX_OFF && mpd_state->conn_state == MPD_CONNECTED) {
                mpd_client_jukebox(mpd_state);
            }
            if (response->length == 0) {
                response->length = snprintf(response->data, MAX_SIZE, "{\"type\": \"result\", \"data\": \"ok\"}");
            }
            break;
        case MPD_API_DATABASE_UPDATE:
            uint_rc = mpd_run_update(mpd_state->conn, NULL);
            if (uint_rc > 0)
                response->length = snprintf(response->data, MAX_SIZE, "{\"type\": \"result\", \"data\": \"ok\"}");
            break;
        case MPD_API_DATABASE_RESCAN:
            uint_rc = mpd_run_rescan(mpd_state->conn, NULL);
            if (uint_rc > 0)
                response->length = snprintf(response->data, MAX_SIZE, "{\"type\": \"result\", \"data\": \"ok\"}");
            break;
        case MPD_API_SMARTPLS_UPDATE_ALL:
            rc = mpd_client_smartpls_update_all(config, mpd_state);
            if (rc == true) {
                response->length = snprintf(response->data, MAX_SIZE, "{\"type\": \"result\", \"data\": \"Smart playlists updated\"}");
            }
            else {
                response->length = snprintf(response->data, MAX_SIZE, "{\"type\": \"error\", \"data\": \"Smart playlists update failed\"}");
            }
            break;
        case MPD_API_SMARTPLS_UPDATE:
            je = json_scanf(request->data, request->length, "{data: {playlist: %Q}}", &p_charbuf1);
            if (je == 1) {
                rc = mpd_client_smartpls_update(config, mpd_state, p_charbuf1);
                if (rc == true) {
                    response->length = snprintf(response->data, MAX_SIZE, "{\"type\": \"result\", \"data\": \"Smart playlist %%{playlist} updated\", \"values\": {\"playlist\": \"%s\"}}", p_charbuf1);
                }
                else {
                    response->length = snprintf(response->data, MAX_SIZE, "{\"type\": \"error\", \"data\": \"Smart playlist update failed\"}");
                }
                FREE_PTR(p_charbuf1);
            }
            break;
        case MPD_API_SMARTPLS_SAVE:
            je = json_scanf(request->data, request->length, "{data: {type: %Q}}", &p_charbuf1);
            response->length = 0;
            rc = false;
            if (je == 1) {
                if (strcmp(p_charbuf1, "sticker") == 0) {
                    je = json_scanf(request->data, request->length, "{data: {playlist: %Q, sticker: %Q, maxentries: %d}}", &p_charbuf2, &p_charbuf3, &int_buf1);
                    if (je == 3) {
                        rc = mpd_client_smartpls_save(config, mpd_state, p_charbuf1, p_charbuf2, p_charbuf3, NULL, int_buf1, 0);
                        FREE_PTR(p_charbuf2);
                        FREE_PTR(p_charbuf3);
                    }
                }
                else if (strcmp(p_charbuf1, "newest") == 0) {
                    je = json_scanf(request->data, request->length, "{data: {playlist: %Q, timerange: %d}}", &p_charbuf2, &int_buf1);
                    if (je == 2) {
                        rc = mpd_client_smartpls_save(config, mpd_state, p_charbuf1, p_charbuf2, NULL, NULL, 0, int_buf1);
                        FREE_PTR(p_charbuf2);
                    }
                }            
                else if (strcmp(p_charbuf1, "search") == 0) {
                    je = json_scanf(request->data, request->length, "{data: {playlist: %Q, tag: %Q, searchstr: %Q}}", &p_charbuf2, &p_charbuf3, &p_charbuf4);
                    if (je == 3) {
                        rc = mpd_client_smartpls_save(config, mpd_state, p_charbuf1, p_charbuf2, p_charbuf3, p_charbuf4, 0, 0);
                        FREE_PTR(p_charbuf2);
                        FREE_PTR(p_charbuf3);                    
                        FREE_PTR(p_charbuf4);
                    }
                }
                FREE_PTR(p_charbuf1);
            }
            if (rc == true) {
                response->length = snprintf(response->data, MAX_SIZE, "{\"type\": \"result\", \"data\": \"ok\"}");
            }
            else {
                response->length = snprintf(response->data, MAX_SIZE, "{\"type\": \"error\", \"data\": \"Failed to save playlist\"}");
            }
            break;
        case MPD_API_SMARTPLS_GET:
            je = json_scanf(request->data, request->length, "{data: {playlist: %Q}}", &p_charbuf1);
            if (je == 1) {
                response->length = mpd_client_smartpls_put(config, response->data, p_charbuf1);
                FREE_PTR(p_charbuf1);
            }
            break;
        case MPD_API_PLAYER_PAUSE:
            if (mpd_run_toggle_pause(mpd_state->conn))
                response->length = snprintf(response->data, MAX_SIZE, "{\"type\": \"result\", \"data\": \"ok\"}");
            else {
                response->length = snprintf(response->data, MAX_SIZE, "{\"type\": \"error\", \"data\": \"Toggling player pause failed.\"}");
                LOG_ERROR("Error mpd_run_toggle_pause()");
            }
            break;
        case MPD_API_PLAYER_PREV:
            if (mpd_run_previous(mpd_state->conn))
                response->length = snprintf(response->data, MAX_SIZE, "{\"type\": \"result\", \"data\": \"ok\"}");
            else {
                response->length = snprintf(response->data, MAX_SIZE, "{\"type\": \"error\", \"data\": \"Goto previous song failed\"}");
                LOG_ERROR("Error mpd_run_previous()");
            }
            break;
        case MPD_API_PLAYER_NEXT:
            if (mpd_run_next(mpd_state->conn))
                response->length = snprintf(response->data, MAX_SIZE, "{\"type\": \"result\", \"data\": \"ok\"}");
            else {
                response->length = snprintf(response->data, MAX_SIZE, "{\"type\": \"error\", \"data\": \"Skip to next song failed\"}");
                LOG_ERROR("Error mpd_run_next()");
            }
            break;
        case MPD_API_PLAYER_PLAY:
            if (mpd_run_play(mpd_state->conn)) {
                response->length = snprintf(response->data, MAX_SIZE, "{\"type\": \"result\", \"data\": \"ok\"}");
            }
            else {
                response->length = snprintf(response->data, MAX_SIZE, "{\"type\": \"error\", \"data\": \"Begin to play failed\"}");
                LOG_ERROR("Error mpd_run_play()");
            }
            break;
        case MPD_API_PLAYER_STOP:
            if (mpd_run_stop(mpd_state->conn))
                response->length = snprintf(response->data, MAX_SIZE, "{\"type\": \"result\", \"data\": \"ok\"}");
            else {
                response->length = snprintf(response->data, MAX_SIZE, "{\"type\": \"error\", \"data\": \"Stop playing failed\"}");
                LOG_ERROR("Error mpd_run_stop()");
            }
            break;
        case MPD_API_QUEUE_CLEAR:
            if (mpd_run_clear(mpd_state->conn))
                response->length = snprintf(response->data, MAX_SIZE, "{\"type\": \"result\", \"data\": \"ok\"}");
            else {
                response->length = snprintf(response->data, MAX_SIZE, "{\"type\": \"error\", \"data\": \"Clearing queue failed\"}");
                LOG_ERROR("Error mpd_run_clear()");
            }
            break;
        case MPD_API_QUEUE_CROP:
            response->length = mpd_client_queue_crop(mpd_state, response->data);
            break;
        case MPD_API_QUEUE_RM_TRACK:
            je = json_scanf(request->data, request->length, "{data: {track:%u}}", &uint_buf1);
            if (je == 1) {
                if (mpd_run_delete_id(mpd_state->conn, uint_buf1))
                    response->length = snprintf(response->data, MAX_SIZE, "{\"type\": \"result\", \"data\": \"ok\"}");
                else {
                    response->length = snprintf(response->data, MAX_SIZE, "{\"type\": \"error\", \"data\": \"Removing track from queue failed\"}");
                    LOG_ERROR("Error mpd_run_delete_id()");
                }
            }
            break;
        case MPD_API_QUEUE_RM_RANGE:
            je = json_scanf(request->data, request->length, "{data: {start: %u, end: %u}}", &uint_buf1, &uint_buf2);
            if (je == 2) {
                if (mpd_run_delete_range(mpd_state->conn, uint_buf1, uint_buf2))
                    response->length = snprintf(response->data, MAX_SIZE, "{\"type\": \"result\", \"data\": \"ok\"}");
                else {
                    response->length = snprintf(response->data, MAX_SIZE, "{\"type\": \"error\", \"data\": \"Removing track range from queue failed\"}");
                    LOG_ERROR("Error mpd_run_delete_range()");
                }
            }
            break;
        case MPD_API_QUEUE_MOVE_TRACK:
            je = json_scanf(request->data, request->length, "{data: {from: %u, to: %u}}", &uint_buf1, &uint_buf2);
            if (je == 2) {
                uint_buf1--;
                uint_buf2--;
                if (uint_buf1 < uint_buf2)
                    uint_buf2--;
                if (mpd_run_move(mpd_state->conn, uint_buf1, uint_buf2))
                    response->length = snprintf(response->data, MAX_SIZE, "{\"type\": \"result\", \"data\": \"ok\"}");
                else {
                    response->length = snprintf(response->data, MAX_SIZE, "{\"type\": \"error\", \"data\": \"Moving track in queue failed\"}");
                    LOG_ERROR("Error mpd_run_move()");
                }
            }
            break;
        case MPD_API_PLAYLIST_MOVE_TRACK:
            je = json_scanf(request->data, request->length, "{data: {plist: %Q, from: %u, to: %u }}", &p_charbuf1, &uint_buf1, &uint_buf2);
            if (je == 3) {
                uint_buf1--;
                uint_buf2--;
                if (uint_buf1 < uint_buf2)
                    uint_buf2--;
                if (mpd_send_playlist_move(mpd_state->conn, p_charbuf1, uint_buf1, uint_buf2)) {
                    mpd_response_finish(mpd_state->conn);
                    response->length = snprintf(response->data, MAX_SIZE, "{\"type\": \"result\", \"data\": \"ok\"}");
                }
                else {
                    response->length = snprintf(response->data, MAX_SIZE, "{\"type\": \"error\", \"data\": \"Moving track in playlist failed\"}");
                    LOG_ERROR("Error mpd_send_playlist_move()");
                }
                FREE_PTR(p_charbuf1);
            }
            break;
        case MPD_API_PLAYER_PLAY_TRACK:
            je = json_scanf(request->data, request->length, "{data: { track:%u}}", &uint_buf1);
            if (je == 1) {
                if (mpd_run_play_id(mpd_state->conn, uint_buf1)) {
                    response->length = snprintf(response->data, MAX_SIZE, "{\"type\": \"result\", \"data\": \"ok\"}");
                }
                else {
                    response->length = snprintf(response->data, MAX_SIZE, "{\"type\": \"error\", \"data\": \"Set playing song failed\"}");
                    LOG_ERROR("Error mpd_run_play_id()");
                }
            }
            break;
        case MPD_API_PLAYER_OUTPUT_LIST:
            response->length = mpd_client_put_outputs(mpd_state, response->data);
            break;
        case MPD_API_PLAYER_TOGGLE_OUTPUT:
            je = json_scanf(request->data, request->length, "{data: {output: %u, state: %u}}", &uint_buf1, &uint_buf2);
            if (je == 2) {
                if (uint_buf2) {
                    if (mpd_run_enable_output(mpd_state->conn, uint_buf1))
                        response->length = snprintf(response->data, MAX_SIZE, "{\"type\": \"result\", \"data\": \"ok\"}");
                    else {
                        response->length = snprintf(response->data, MAX_SIZE, "{\"type\": \"error\", \"data\": \"Enabling output failed\"}");
                        LOG_ERROR("Error mpd_run_enable_output()");
                    }
                }
                else {
                    if (mpd_run_disable_output(mpd_state->conn, uint_buf1))
                        response->length = snprintf(response->data, MAX_SIZE, "{\"type\": \"result\", \"data\": \"ok\"}");
                    else {
                        response->length = snprintf(response->data, MAX_SIZE, "{\"type\": \"error\", \"data\": \"Disabling output failed\"}");
                        LOG_ERROR("Error mpd_run_disable_output()");
                    }
                }
            }
            break;
        case MPD_API_PLAYER_VOLUME_SET:
            je = json_scanf(request->data, request->length, "{data: {volume:%u}}", &uint_buf1);
            if (je == 1) {
                if (mpd_run_set_volume(mpd_state->conn, uint_buf1))
                    response->length = snprintf(response->data, MAX_SIZE, "{\"type\": \"result\", \"data\": \"ok\"}");
                else {
                    response->length = snprintf(response->data, MAX_SIZE, "{\"type\": \"error\", \"data\": \"Setting volume failed\"}");
                    LOG_ERROR("MPD_API_PLAYER_PLAY_TRACK: Error mpd_run_set_volume()");
                }
            }
            break;
        case MPD_API_PLAYER_VOLUME_GET:
            response->length = mpd_client_put_volume(mpd_state, response->data);
            break;            
        case MPD_API_PLAYER_SEEK:
            je = json_scanf(request->data, request->length, "{data: {songid: %u, seek: %u}}", &uint_buf1, &uint_buf2);
            if (je == 2) {
                if (mpd_run_seek_id(mpd_state->conn, uint_buf1, uint_buf2))
                    response->length = snprintf(response->data, MAX_SIZE, "{\"type\": \"result\", \"data\": \"ok\"}");
                else {
                    response->length = snprintf(response->data, MAX_SIZE, "{\"type\": \"error\", \"data\": \"Seeking song failed\"}");
                    LOG_ERROR("Error mpd_run_seek_id()");
                }
            }
            break;
        case MPD_API_QUEUE_LIST: {
            t_tags *tagcols = (t_tags *)malloc(sizeof(t_tags));
            assert(tagcols);
            je = json_scanf(request->data, request->length, "{data: {offset: %u, cols: %M}}", &uint_buf1, json_to_tags, tagcols);
            if (je == 2) {
                response->length = mpd_client_put_queue(mpd_state, response->data, uint_buf1, tagcols);
            }
            free(tagcols);
            break;
        }
        case MPD_API_QUEUE_LAST_PLAYED: {
            t_tags *tagcols = (t_tags *)malloc(sizeof(t_tags));
            assert(tagcols);
            je = json_scanf(request->data, request->length, "{data: {offset: %u, cols: %M}}", &uint_buf1, json_to_tags, tagcols);
            if (je == 2) {
                response->length = mpd_client_put_last_played_songs(config, mpd_state, response->data, uint_buf1, tagcols);
            }
            free(tagcols);
            break;
        }
        case MPD_API_PLAYER_CURRENT_SONG: {
            response->length = mpd_client_put_current_song(config, mpd_state, response->data);
            break;
        }
        case MPD_API_DATABASE_SONGDETAILS:
            je = json_scanf(request->data, request->length, "{data: { uri: %Q}}", &p_charbuf1);
            if (je == 1) {
                response->length = mpd_client_put_songdetails(config, mpd_state, response->data, p_charbuf1);
                FREE_PTR(p_charbuf1);
            }
            break;
        case MPD_API_DATABASE_FINGERPRINT:
            je = json_scanf(request->data, request->length, "{data: { uri: %Q}}", &p_charbuf1);
            if (je == 1) {
                response->length = mpd_client_put_fingerprint(mpd_state, response->data, p_charbuf1);
                FREE_PTR(p_charbuf1);
            }
            break;
        case MPD_API_DATABASE_TAG_LIST:
            je = json_scanf(request->data, request->length, "{data: {offset: %u, filter: %Q, tag: %Q}}", &uint_buf1, &p_charbuf1, &p_charbuf2);
            if (je == 3) {
                response->length = mpd_client_put_db_tag(mpd_state, response->data, uint_buf1, p_charbuf2, "", "", p_charbuf1);
                FREE_PTR(p_charbuf1);
                FREE_PTR(p_charbuf2);
            }
            break;
        case MPD_API_DATABASE_TAG_ALBUM_LIST:
            je = json_scanf(request->data, request->length, "{data: {offset: %u, filter: %Q, search: %Q, tag: %Q}}", 
                &uint_buf1, &p_charbuf1, &p_charbuf2, &p_charbuf3);
            if (je == 4) {
                response->length = mpd_client_put_db_tag(mpd_state, response->data, uint_buf1, "Album", p_charbuf3, p_charbuf2, p_charbuf1);
                FREE_PTR(p_charbuf1);
                FREE_PTR(p_charbuf2);
                FREE_PTR(p_charbuf3);
            }
            break;
        case MPD_API_DATABASE_TAG_ALBUM_TITLE_LIST: {
            t_tags *tagcols = (t_tags *)malloc(sizeof(t_tags));
            assert(tagcols);
            je = json_scanf(request->data, request->length, "{data: {album: %Q, search: %Q, tag: %Q, cols: %M}}", 
                &p_charbuf1, &p_charbuf2, &p_charbuf3, json_to_tags, tagcols);
            if (je == 4) {
                response->length = mpd_client_put_songs_in_album(config, mpd_state, response->data, p_charbuf1, p_charbuf2, p_charbuf3, tagcols);
                FREE_PTR(p_charbuf1);
                FREE_PTR(p_charbuf2);
                FREE_PTR(p_charbuf3);
            }
            free(tagcols);
            break;
        }
        case MPD_API_PLAYLIST_RENAME:
            je = json_scanf(request->data, request->length, "{data: {from: %Q, to: %Q}}", &p_charbuf1, &p_charbuf2);
            if (je == 2) {
                response->length = mpd_client_rename_playlist(config, mpd_state, response->data, p_charbuf1, p_charbuf2);
                FREE_PTR(p_charbuf1);
                FREE_PTR(p_charbuf2);
            }
            break;            
        case MPD_API_PLAYLIST_LIST:
            je = json_scanf(request->data, request->length, "{data: {offset: %u, filter: %Q}}", &uint_buf1, &p_charbuf1);
            if (je == 2) {
                response->length = mpd_client_put_playlists(config, mpd_state, response->data, uint_buf1, p_charbuf1);
                FREE_PTR(p_charbuf1);
            }
            break;
        case MPD_API_PLAYLIST_CONTENT_LIST: {
            t_tags *tagcols = (t_tags *)malloc(sizeof(t_tags));
            assert(tagcols);
            je = json_scanf(request->data, request->length, "{data: {uri: %Q, offset:%u, filter:%Q, cols: %M}}", 
                &p_charbuf1, &uint_buf1, &p_charbuf2, json_to_tags, tagcols);
            if (je == 4) {
                response->length = mpd_client_put_playlist_list(config, mpd_state, response->data, p_charbuf1, uint_buf1, p_charbuf2, tagcols);
                FREE_PTR(p_charbuf1);
                FREE_PTR(p_charbuf2);
            }
            free(tagcols);
            break;
        }
        case MPD_API_PLAYLIST_ADD_TRACK:
            je = json_scanf(request->data, request->length, "{data: {plist:%Q, uri:%Q}}", &p_charbuf1, &p_charbuf2);
            if (je == 2) {
                if (mpd_run_playlist_add(mpd_state->conn, p_charbuf1, p_charbuf2)) {
                    response->length = snprintf(response->data, MAX_SIZE, "{\"type\": \"result\", \"data\": \"Added %%{uri} to playlist %%{playlist}\", "
                        "\"values\": {\"uri\": \"%s\", \"playlist\": \"%s\"}}", p_charbuf2, p_charbuf1);
                }
                else {
                    response->length = snprintf(response->data, MAX_SIZE, "{\"type\": \"error\", \"data\": \"Adding song to playlist failed\"}");
                    LOG_ERROR("Error mpd_run_playlist_add");
                }
                FREE_PTR(p_charbuf1);
                FREE_PTR(p_charbuf2);                
            }
            break;
        case MPD_API_PLAYLIST_CLEAR:
            je = json_scanf(request->data, request->length, "{data: {uri:%Q}}", &p_charbuf1);
            if (je == 1) {
                if (mpd_run_playlist_clear(mpd_state->conn, p_charbuf1)) {
                    response->length = snprintf(response->data, MAX_SIZE, "{\"type\": \"result\", \"data\": \"ok\"}");
                }
                else {
                    response->length = snprintf(response->data, MAX_SIZE, "{\"type\": \"error\", \"data\": \"Clearing playlist failed\"}");
                    LOG_ERROR("Error mpd_run_playlist_clear");
                }
                FREE_PTR(p_charbuf1);
            }
            break;
        case MPD_API_PLAYLIST_RM_TRACK:
            je = json_scanf(request->data, request->length, "{data: {uri:%Q, track:%u}}", &p_charbuf1, &uint_buf1);
            if (je == 2) {
                if (mpd_run_playlist_delete(mpd_state->conn, p_charbuf1, uint_buf1)) {
                    response->length = snprintf(response->data, MAX_SIZE, "{\"type\": \"result\", \"data\": \"ok\"}");
                }
                else {
                    response->length = snprintf(response->data, MAX_SIZE, "{\"type\": \"error\", \"data\": \"Removing track from playlist failed\"}");
                    LOG_ERROR("Error mpd_run_playlist_delete");
                }
                FREE_PTR(p_charbuf1);
            }
            break;
        case MPD_API_DATABASE_FILESYSTEM_LIST: {
            t_tags *tagcols = (t_tags *)malloc(sizeof(t_tags));
            assert(tagcols);
            je = json_scanf(request->data, request->length, "{data: {offset:%u, filter:%Q, path:%Q, cols: %M}}", 
                &uint_buf1, &p_charbuf1, &p_charbuf2, json_to_tags, tagcols);
            if (je == 4) {
                response->length = mpd_client_put_browse(config, mpd_state, response->data, p_charbuf2, uint_buf1, p_charbuf1, tagcols);
                FREE_PTR(p_charbuf1);
                FREE_PTR(p_charbuf2);
            }
            free(tagcols);
            break;
        }
        case MPD_API_QUEUE_ADD_TRACK_AFTER:
            je = json_scanf(request->data, request->length, "{data: {uri:%Q, to:%d}}", &p_charbuf1, &int_buf1);
            if (je == 2) {
                int_rc = mpd_run_add_id_to(mpd_state->conn, p_charbuf1, int_buf1);
                if (int_rc > -1 ) 
                    response->length = snprintf(response->data, MAX_SIZE, "{\"type\": \"result\", \"data\": \"ok\"}");
                else {
                    response->length = snprintf(response->data, MAX_SIZE, "{\"type\": \"error\", \"data\": \"Adding song to queue failed\"}");
                    LOG_ERROR("Error mpd_run_add_id_to()");
                }
                FREE_PTR(p_charbuf1);
            }
            break;
        case MPD_API_QUEUE_REPLACE_TRACK:
            je = json_scanf(request->data, request->length, "{data: {uri:%Q }}", &p_charbuf1);
            if (je == 1) {
                if (!mpd_run_clear(mpd_state->conn)) {
                    response->length = snprintf(response->data, MAX_SIZE, "{\"type\": \"error\", \"data\": \"Clearing queue failed\"}");
                    LOG_ERROR("Error mpd_run_clear");
                }
                else if (!mpd_run_add(mpd_state->conn, p_charbuf1)) {
                    response->length = snprintf(response->data, MAX_SIZE, "{\"type\": \"error\", \"data\": \"Adding song to queue failed\"}");
                    LOG_ERROR("Error mpd_run_add");
                }
                else if (!mpd_run_play(mpd_state->conn)) {
                    response->length = snprintf(response->data, MAX_SIZE, "{\"type\": \"error\", \"data\": \"Can not start playing\"}");
                    LOG_ERROR("Error mpd_run_play");
                }
                else
                    response->length = snprintf(response->data, MAX_SIZE, "{\"type\": \"result\", \"data\": \"ok\"}");
                FREE_PTR(p_charbuf1);
            }
            break;
        case MPD_API_QUEUE_ADD_TRACK:
            je = json_scanf(request->data, request->length, "{data: {uri:%Q}}", &p_charbuf1);
            if (je == 1) {
                if (mpd_run_add(mpd_state->conn, p_charbuf1))
                    response->length = snprintf(response->data, MAX_SIZE, "{\"type\": \"result\", \"data\": \"ok\"}");
                else {
                    response->length = snprintf(response->data, MAX_SIZE, "{\"type\": \"error\", \"data\": \"Adding song to queue failed\"}");
                    LOG_ERROR("Error mpd_run_add");
                }
                FREE_PTR(p_charbuf1);
            }
            break;
        case MPD_API_QUEUE_ADD_PLAY_TRACK:
            je = json_scanf(request->data, request->length, "{data: {uri:%Q}}", &p_charbuf1);
            if (je == 1) {
                int_buf1 = mpd_run_add_id(mpd_state->conn, p_charbuf1);
                if (int_buf1 != -1) {
                    if (mpd_run_play_id(mpd_state->conn, int_buf1)) {
                        response->length = snprintf(response->data, MAX_SIZE, "{\"type\": \"result\", \"data\": \"ok\"}");
                    }
                    else {
                        response->length = snprintf(response->data, MAX_SIZE, "{\"type\": \"error\", \"data\": \"Can not start playing\"}");
                        LOG_ERROR("Error mpd_run_play_id()");
                    }
                }
                else {
                    response->length = snprintf(response->data, MAX_SIZE, "{\"type\": \"error\", \"data\": \"Adding song to queue failed\"}");
                    LOG_ERROR("Error mpd_run_add_id");
                }
                FREE_PTR(p_charbuf1);
            }
            break;
        case MPD_API_QUEUE_REPLACE_PLAYLIST:
            je = json_scanf(request->data, request->length, "{data: {plist:%Q}}", &p_charbuf1);
            if (je == 1) {
                if (!mpd_run_clear(mpd_state->conn)) {
                    response->length = snprintf(response->data, MAX_SIZE, "{\"type\": \"error\", \"data\": \"Clearing queue failed\"}");
                    LOG_ERROR("Error mpd_run_clear");                
                }
                else if (!mpd_run_load(mpd_state->conn, p_charbuf1)) {
                    response->length = snprintf(response->data, MAX_SIZE, "{\"type\": \"error\", \"data\": \"Adding playlist to queue failed\"}");
                    LOG_ERROR("Error mpd_run_load");
                }
                else if (!mpd_run_play(mpd_state->conn)) {
                    response->length = snprintf(response->data, MAX_SIZE, "{\"type\": \"error\", \"data\": \"Can not start playing\"}");
                    LOG_ERROR("Error mpd_run_play");
                }
                else {
                    response->length = snprintf(response->data, MAX_SIZE, "{\"type\": \"result\", \"data\": \"ok\"}");
                }
                FREE_PTR(p_charbuf1);
            }
            break;
        case MPD_API_QUEUE_ADD_RANDOM:
            je = json_scanf(request->data, request->length, "{data: {mode:%u, playlist:%Q, quantity:%d}}", &uint_buf1, &p_charbuf1, &int_buf1);
            if (je == 3) {
                rc = mpd_client_jukebox_add(mpd_state, int_buf1, uint_buf1, p_charbuf1);
                FREE_PTR(p_charbuf1);
                if (rc == true) {
                    response->length = snprintf(response->data, MAX_SIZE, "{\"type\": \"result\", \"data\": \"Sucessfully added random songs to queue\"}");
                }
                else {
                    response->length = snprintf(response->data, MAX_SIZE, "{\"type\": \"error\", \"data\": \"Adding random songs to queue failed\"}");
                }
            }
            break;
        case MPD_API_QUEUE_ADD_PLAYLIST:
            je = json_scanf(request->data, request->length, "{data: {plist:%Q}}", &p_charbuf1);
            if (je == 1) {
                if (mpd_run_load(mpd_state->conn, p_charbuf1)) {
                    response->length = snprintf(response->data, MAX_SIZE, "{\"type\": \"result\", \"data\": \"ok\"}");
                }
                else {
                    response->length = snprintf(response->data, MAX_SIZE, "{\"type\": \"error\", \"data\": \"Adding playlist to queue failed\"}");
                    LOG_ERROR("Error mpd_run_load");
                }
                FREE_PTR(p_charbuf1);
            }
            break;
        case MPD_API_QUEUE_SAVE:
            je = json_scanf(request->data, request->length, "{ data: {plist:%Q}}", &p_charbuf1);
            if (je == 1) {
                if (mpd_run_save(mpd_state->conn, p_charbuf1))
                    response->length = snprintf(response->data, MAX_SIZE, "{\"type\": \"result\", \"data\": \"ok\"}");
                else {
                    response->length = snprintf(response->data, MAX_SIZE, "{\"type\": \"error\", \"data\": \"Saving queue as playlist failed\"}");
                    LOG_ERROR("Error mpd_run_save");
                }
                FREE_PTR(p_charbuf1);
            }
            break;
        case MPD_API_QUEUE_SEARCH: {
            t_tags *tagcols = (t_tags *)malloc(sizeof(t_tags));
            assert(tagcols);
            je = json_scanf(request->data, request->length, "{data: {offset:%u, filter:%Q, searchstr:%Q, cols: %M}}", 
                &uint_buf1, &p_charbuf1, &p_charbuf2, json_to_tags, tagcols);
            if (je == 4) {
                response->length = mpd_client_search_queue(mpd_state, response->data, p_charbuf1, uint_buf1, p_charbuf2, tagcols);
                FREE_PTR(p_charbuf1);
                FREE_PTR(p_charbuf2);
            }
            free(tagcols);
            break;
        }
        case MPD_API_DATABASE_SEARCH: {
            t_tags *tagcols = (t_tags *)malloc(sizeof(t_tags));
            assert(tagcols);
            je = json_scanf(request->data, request->length, "{data: {searchstr:%Q, filter:%Q, plist:%Q, offset:%u, cols: %M}}", 
                &p_charbuf1, &p_charbuf2, &p_charbuf3, &uint_buf1, json_to_tags, tagcols);
            if (je == 5) {
                response->length = mpd_client_search(mpd_state, response->data, p_charbuf1, p_charbuf2, p_charbuf3, uint_buf1, tagcols);
                FREE_PTR(p_charbuf1);
                FREE_PTR(p_charbuf2);
                FREE_PTR(p_charbuf3);
            }
            free(tagcols);
            break;
        }
        case MPD_API_DATABASE_SEARCH_ADV: {
            t_tags *tagcols = (t_tags *)malloc(sizeof(t_tags));
            assert(tagcols);
            je = json_scanf(request->data, request->length, "{data: {expression:%Q, sort:%Q, sortdesc:%B, plist:%Q, offset:%u, cols: %M}}", 
                &p_charbuf1, &p_charbuf2, &bool_buf, &p_charbuf3, &uint_buf1, json_to_tags, tagcols);
            if (je == 6) {
                response->length = mpd_client_search_adv(mpd_state, response->data, p_charbuf1, p_charbuf2, bool_buf, NULL, p_charbuf3, uint_buf1, tagcols);
                FREE_PTR(p_charbuf1);
                FREE_PTR(p_charbuf2);
                FREE_PTR(p_charbuf3);
            }
            free(tagcols);
            break;
        }
        case MPD_API_QUEUE_SHUFFLE:
            if (mpd_run_shuffle(mpd_state->conn))
                response->length = snprintf(response->data, MAX_SIZE, "{\"type\": \"result\", \"data\": \"ok\"}");
            else {
                response->length = snprintf(response->data, MAX_SIZE, "{\"type\": \"error\", \"data\": \"Shuffling queue failed\"}");
                LOG_ERROR("Error mpd_run_shuffle");
            }
            break;
        case MPD_API_PLAYLIST_RM:
            je = json_scanf(request->data, request->length, "{data: {uri:%Q}}", &p_charbuf1);
            if (je == 1) {
                //remove smart playlist
                if (validate_string(p_charbuf1) == false) {
                    response->length = snprintf(response->data, MAX_SIZE, "{\"type\": \"error\", \"data\": \"Invalid filename\"}");
                    break;
                }
                size_t pl_file_len = config->varlibdir_len + strlen(p_charbuf1) + 11;
                char pl_file[pl_file_len];
                snprintf(pl_file, pl_file_len, "%s/smartpls/%s", config->varlibdir, p_charbuf1);
                if (access(pl_file, F_OK ) != -1 ) {
                    if (unlink(pl_file) == -1) {
                        response->length = snprintf(response->data, MAX_SIZE, "{\"type\": \"error\", \"data\": \"Deleting smart playlist failed\"}");
                        LOG_ERROR("Deleting smart playlist failed");
                        FREE_PTR(p_charbuf1);
                        break;
                    }
                }
                //remove mpd playlist
                if (mpd_run_rm(mpd_state->conn, p_charbuf1)) {
                    response->length = snprintf(response->data, MAX_SIZE, "{\"type\": \"result\", \"data\": \"ok\"}");
                }
                else {
                    response->length = snprintf(response->data, MAX_SIZE, "{\"type\": \"error\", \"data\": \"Deleting playlist failed\"}");
                    LOG_ERROR("Error mpd_run_rm()");
                }
                FREE_PTR(p_charbuf1);
            }
            break;
        case MPD_API_SETTINGS_GET:
            response->length = mpd_client_put_settings(mpd_state, response->data);
            break;
        case MPD_API_DATABASE_STATS:
            response->length = mpd_client_put_stats(mpd_state, response->data);
            break;
        default:
            response->length = snprintf(response->data, MAX_SIZE, "{\"type\": \"error\", \"data\": \"Unknown request\"}");
            LOG_ERROR("Unknown API request: %.*s", request->length, request->data);
    }

    if (mpd_state->conn_state == MPD_CONNECTED && mpd_connection_get_error(mpd_state->conn) != MPD_ERROR_SUCCESS) {
        LOG_ERROR("MPD error: %s", mpd_connection_get_error_message(mpd_state->conn));
        response->length = snprintf(response->data, MAX_SIZE, "{\"type\":\"error\", \"data\": \"%s\"}", 
            mpd_connection_get_error_message(mpd_state->conn));

        /* Try to recover error */
        if (!mpd_connection_clear_error(mpd_state->conn))
            mpd_state->conn_state = MPD_FAILURE;
    }

    if (response->length == 0) {
        response->length = snprintf(response->data, MAX_SIZE, "{\"type\": \"error\", \"data\": \"No response for cmd_id %%{cmdId}\", \"values\": {\"cmdId\": %u}}", request->cmd_id);
    }
    if (response->conn_id > -1) {
        LOG_DEBUG("Push response to queue for connection %lu: %s", request->conn_id, response->data);
        tiny_queue_push(web_server_queue, response);
    }
    else {
        FREE_PTR(response);
    }
    FREE_PTR(request);
}

static void mpd_client_notify(const char *message, const size_t len) {
    LOG_DEBUG("Push websocket notify to queue: %s", message);
    t_work_result *response = (t_work_result *)malloc(sizeof(t_work_result));
    assert(response);
    response->conn_id = 0;
    response->length = copy_string(response->data, message, MAX_SIZE, len);
    tiny_queue_push(web_server_queue, response);
}

static void mpd_client_parse_idle(t_config *config, t_mpd_state *mpd_state, int idle_bitmask) {
    size_t len = 0;
    char buffer[MAX_SIZE];
    
    for (unsigned j = 0;; j++) {
        enum mpd_idle idle_event = 1 << j;
        const char *idle_name = mpd_idle_name(idle_event);
        if (idle_name == NULL) {
            break;
        }
        if (idle_bitmask & idle_event) {
            LOG_VERBOSE("MPD idle event: %s", idle_name);
            switch(idle_event) {
                case MPD_IDLE_DATABASE:
                    len = snprintf(buffer, MAX_SIZE, "{\"type\": \"update_database\"}");
                    mpd_client_smartpls_update_all(config, mpd_state);
                    break;
                case MPD_IDLE_STORED_PLAYLIST:
                    len = snprintf(buffer, MAX_SIZE, "{\"type\": \"update_stored_playlist\"}");
                    break;
                case MPD_IDLE_QUEUE:
                    len = mpd_client_get_queue_state(mpd_state, buffer);
                    //jukebox enabled
                    if (mpd_state->jukebox_mode != JUKEBOX_OFF && mpd_state->queue_length < 1) {
                        mpd_client_jukebox(mpd_state);
                    }
                    //autoPlay enabled
                    if (mpd_state->auto_play == true && mpd_state->queue_length > 1) {
                        if (mpd_state->state != MPD_STATE_PLAY) {
                            LOG_VERBOSE("AutoPlay enabled, start playing");
                            if (!mpd_run_play(mpd_state->conn)) {
                                LOG_ERROR_AND_RECOVER("mpd_run_play");
                            }
                        }
                    }
                    break;
                case MPD_IDLE_PLAYER:
                    //get and put mpd state                
                    len = mpd_client_get_state(mpd_state, buffer);
                    //song has changed
                    if (mpd_state->song_id != mpd_state->last_song_id && mpd_state->last_skipped_id != mpd_state->last_song_id  && mpd_state->last_song_uri != NULL) {
                        time_t now = time(NULL);
                        if (mpd_state->feat_sticker && mpd_state->last_song_end_time > now) {
                            //last song skipped
                            time_t elapsed = now - mpd_state->last_song_start_time;
                            if (elapsed > 10 && mpd_state->last_song_start_time > 0) {
                                LOG_DEBUG("Song %s skipped", mpd_state->last_song_uri);
                                mpd_client_count_song_uri(mpd_state, mpd_state->last_song_uri, "skipCount", 1);
                                mpd_client_last_skipped_song_uri(mpd_state, mpd_state->last_song_uri);
                                mpd_state->last_skipped_id = mpd_state->last_song_id;
                            }
                        }
                    }
                    break;
                case MPD_IDLE_MIXER:
                    len = mpd_client_put_volume(mpd_state, buffer);
                    break;
                case MPD_IDLE_OUTPUT:
                    len = snprintf(buffer, MAX_SIZE, "{\"type\": \"update_outputs\"}");
                    break;
                case MPD_IDLE_OPTIONS:
                    mpd_client_get_queue_state(mpd_state, NULL);
                    len = snprintf(buffer, MAX_SIZE, "{\"type\": \"update_options\"}");
                    break;
                case MPD_IDLE_UPDATE:
                    len = mpd_client_get_updatedb_state(mpd_state, buffer);
                    break;
                case MPD_IDLE_STICKER:
                    //not used and disabled in idlemask
                    len = snprintf(buffer, MAX_SIZE, "{\"type\": \"update_sticker\"}");
                    break;
                case MPD_IDLE_SUBSCRIPTION:
                    if (mpd_state->love == true) {
                        bool old_love = mpd_state->feat_love;
                        mpd_client_feature_love(mpd_state);
                        if (old_love != mpd_state->feat_love) {
                            len = snprintf(buffer, MAX_SIZE, "{\"type\": \"update_options\"}");
                        }
                    }
                    break;
                case MPD_IDLE_MESSAGE:
                    //not used and disabled in idlemask
                    len = snprintf(buffer, MAX_SIZE, "{\"type\": \"update_message\"}");
                    break;
                default:
                    len = 0;
            }
            if (len > 0) {
                mpd_client_notify(buffer, len);
            }
        }
    }
}

static void mpd_client_feature_tags(t_mpd_state *mpd_state) {
    size_t max_len = 1024;
    char logline[max_len];
    size_t len = 0;
    char s[] = ",";
    char *taglist = strdup(mpd_state->taglist);
    char *searchtaglist = strdup(mpd_state->searchtaglist);    
    char *browsetaglist = strdup(mpd_state->browsetaglist);
    char *token = NULL;
    char *rest = NULL;
    struct mpd_pair *pair;

    mpd_state->mpd_tag_types_len = 0;
    memset(mpd_state->mpd_tag_types, 0, sizeof(mpd_state->mpd_tag_types));
    mpd_state->mympd_tag_types_len = 0;
    memset(mpd_state->mympd_tag_types, 0, sizeof(mpd_state->mympd_tag_types));
    mpd_state->search_tag_types_len = 0;
    memset(mpd_state->search_tag_types, 0, sizeof(mpd_state->search_tag_types));
    mpd_state->browse_tag_types_len = 0;
    memset(mpd_state->browse_tag_types, 0, sizeof(mpd_state->browse_tag_types));
    
    len = snprintf(logline, max_len, "MPD supported tags: ");
    if (mpd_send_list_tag_types(mpd_state->conn)) {
        while ((pair = mpd_recv_tag_type_pair(mpd_state->conn)) != NULL) {
            enum mpd_tag_type tag = mpd_tag_name_parse(pair->value);
            if (tag != MPD_TAG_UNKNOWN) {
                len += snprintf(logline + len, max_len - len, "%s ", pair->value);
                mpd_state->mpd_tag_types[mpd_state->mpd_tag_types_len++] = tag;
            }
            else {
                LOG_WARN("Unknown tag %s (libmpdclient to old)", pair->value);
            }
            mpd_return_pair(mpd_state->conn, pair);
        }
        mpd_response_finish(mpd_state->conn);
    }
    else {
        LOG_ERROR_AND_RECOVER("mpd_send_list_tag_types");
    }

    if (mpd_state->mpd_tag_types_len == 0) {
        len += snprintf(logline + len, max_len -len, "none");
        LOG_INFO(logline);
        LOG_INFO("Tags are disabled");
        mpd_state->feat_tags = false;
    }
    else {
        mpd_state->feat_tags = true;
        LOG_INFO(logline);
        len = snprintf(logline, max_len, "myMPD enabled tags: ");
        token = strtok_r(taglist, s, &rest);
        while (token != NULL) {
            enum mpd_tag_type tag = mpd_tag_name_iparse(token);
            if (tag == MPD_TAG_UNKNOWN) {
                LOG_WARN("Unknown tag %s", token);
            }
            else {
                if (mpd_client_tag_exists(mpd_state->mpd_tag_types, mpd_state->mpd_tag_types_len, tag) == true) {
                    len += snprintf(logline + len, max_len - len, "%s ", mpd_tag_name(tag));
                    mpd_state->mympd_tag_types[mpd_state->mympd_tag_types_len++] = tag;
                }
                else {
                    LOG_DEBUG("Disabling tag %s", mpd_tag_name(tag));
                }
            }
            token = strtok_r(NULL, s, &rest);
        }
        LOG_INFO(logline);
        #if LIBMPDCLIENT_CHECK_VERSION(2,12,0)
        if (mpd_connection_cmp_server_version(mpd_state->conn, 0, 21, 0) >= 0) {
            LOG_VERBOSE("Enabling mpd tag types");
            if (mpd_command_list_begin(mpd_state->conn, false)) {
                mpd_send_clear_tag_types(mpd_state->conn);
                mpd_send_enable_tag_types(mpd_state->conn, mpd_state->mympd_tag_types, mpd_state->mympd_tag_types_len);
                if (!mpd_command_list_end(mpd_state->conn)) {
                    LOG_ERROR_AND_RECOVER("mpd_command_list_end");
                }
            }
            else {
                LOG_ERROR_AND_RECOVER("mpd_command_list_begin");
            }
            mpd_response_finish(mpd_state->conn);
        }
        #endif
        len = snprintf(logline, max_len, "myMPD enabled searchtags: ");
        token = strtok_r(searchtaglist, s, &rest);
        while (token != NULL) {
            enum mpd_tag_type tag = mpd_tag_name_iparse(token);
            if (tag == MPD_TAG_UNKNOWN) {
                LOG_WARN("Unknown tag %s", token);
            }
            else {
                if (mpd_client_tag_exists(mpd_state->mympd_tag_types, mpd_state->mympd_tag_types_len, tag) == true) {
                    len += snprintf(logline + len, max_len - len, "%s ", mpd_tag_name(tag));
                    mpd_state->search_tag_types[mpd_state->search_tag_types_len++] = tag;
                }
                else {
                    LOG_DEBUG("Disabling tag %s", mpd_tag_name(tag));
                }
            }
            token = strtok_r(NULL, s, &rest);
        }
        LOG_INFO(logline);
        len = snprintf(logline, max_len, "myMPD enabled browsetags: ");
        token = strtok_r(browsetaglist, s, &rest);
        while (token != NULL) {
            enum mpd_tag_type tag = mpd_tag_name_iparse(token);
            if (tag == MPD_TAG_UNKNOWN) {
                LOG_WARN("Unknown tag %s", token);
            }
            else {
                if (mpd_client_tag_exists(mpd_state->mympd_tag_types, mpd_state->mympd_tag_types_len, tag) == true) {
                    len += snprintf(logline + len, max_len - len, "%s ", mpd_tag_name(tag));
                    mpd_state->browse_tag_types[mpd_state->browse_tag_types_len++] = tag;
                }
                else {
                    LOG_DEBUG("Disabling tag %s", mpd_tag_name(tag));
                }
            }
            token = strtok_r(NULL, s, &rest);
        }
        LOG_INFO(logline);
    }
    FREE_PTR(taglist);
    FREE_PTR(searchtaglist);
    FREE_PTR(browsetaglist);

}

static void mpd_client_feature_love(t_mpd_state *mpd_state) {
    struct mpd_pair *pair;
    mpd_state->feat_love = false;
    if (mpd_state->love == true) {
        if (mpd_send_channels(mpd_state->conn)) {
            while ((pair = mpd_recv_channel_pair(mpd_state->conn)) != NULL) {
                if (strcmp(pair->value, mpd_state->love_channel) == 0) {
                    mpd_state->feat_love = true;
                }
                mpd_return_pair(mpd_state->conn, pair);            
            }
        }
        mpd_response_finish(mpd_state->conn);
        if (mpd_state->feat_love == false) {
            LOG_WARN("Disabling featLove, channel %s not found", mpd_state->love_channel);
        }
        else {
            LOG_INFO("Enabling featLove, channel %s found", mpd_state->love_channel);
        }
    }
}

static void mpd_client_feature_music_directory(t_mpd_state *mpd_state) {
    struct mpd_pair *pair;
    mpd_state->feat_library = false;
    mpd_state->feat_coverimage = mpd_state->coverimage;
    FREE_PTR(mpd_state->music_directory_value);
    mpd_state->music_directory_value = strdup("");

    if (strncmp(mpd_state->mpd_host, "/", 1) == 0 && strncmp(mpd_state->music_directory, "auto", 4) == 0) {
        //get musicdirectory from mpd
        if (mpd_send_command(mpd_state->conn, "config", NULL)) {
            while ((pair = mpd_recv_pair(mpd_state->conn)) != NULL) {
                if (strcmp(pair->name, "music_directory") == 0) {
                    if (strncmp(pair->value, "smb://", 6) != 0 && strncmp(pair->value, "nfs://", 6) != 0) {
                        FREE_PTR(mpd_state->music_directory_value);
                        mpd_state->music_directory_value = strdup(pair->value);
                    }
                }
                mpd_return_pair(mpd_state->conn, pair);
            }
            mpd_response_finish(mpd_state->conn);
        }
        else {
            LOG_ERROR_AND_RECOVER("config");
        }
    }
    else if (strncmp(mpd_state->music_directory, "/", 1) == 0) {
        FREE_PTR(mpd_state->music_directory_value);
        mpd_state->music_directory_value = strdup(mpd_state->music_directory);
    }
    else {
        //none or garbage, empty music_directory_value
    }
    
    //set feat_library
    if (strlen(mpd_state->music_directory_value) == 0) {
        LOG_WARN("Disabling featLibrary support");
        mpd_state->feat_library = false;
    }
    else if (testdir("MPD music_directory", mpd_state->music_directory_value)) {
        LOG_INFO("Enabling featLibrary support");
        mpd_state->feat_library = true;
    }
    else {
        LOG_WARN("Disabling featLibrary support");
        mpd_state->feat_library = false;
        FREE_PTR(mpd_state->music_directory_value);
        mpd_state->music_directory_value = strdup("");
    }
    
    if (mpd_state->feat_library == false) {
        LOG_WARN("Disabling coverimage support");
        mpd_state->feat_coverimage = false;
    }

    //push music_directory setting to web_server_queue
    t_work_result *web_server_response = (t_work_result *)malloc(sizeof(t_work_result));
    assert(web_server_response);
    web_server_response->conn_id = -1;
    web_server_response->length = snprintf(web_server_response->data, MAX_SIZE, 
        "{\"musicDirectory\":\"%s\", \"featLibrary\": %s}",
        mpd_state->music_directory_value,
        mpd_state->feat_library == true ? "true" : "false"
    );
    tiny_queue_push(web_server_queue, web_server_response);
}

static void json_to_tags(const char *str, int len, void *user_data) {
    struct json_token t;
    int i;
    t_tags *tags = (t_tags *) user_data;
    tags->len = 0;
    for (i = 0; json_scanf_array_elem(str, len, "", i, &t) > 0; i++) {
        char token[t.len + 1];
        snprintf(token, t.len + 1, "%.*s", t.len, t.ptr);
        enum mpd_tag_type tag = mpd_tag_name_iparse(token);
        if (tag != MPD_TAG_UNKNOWN) {
            tags->tags[tags->len++] = tag;
        }
    }
}

static bool mpd_client_tag_exists(const enum mpd_tag_type tag_types[64], const size_t tag_types_len, const enum mpd_tag_type tag) {
    for (size_t i = 0; i < tag_types_len; i++) {
        if (tag_types[i] == tag) {
	    return true;
	}
   }
   return false;
}

static void mpd_client_mpd_features(t_mpd_state *mpd_state) {
    struct mpd_pair *pair;

    mpd_state->protocol = mpd_connection_get_server_version(mpd_state->conn);
    LOG_INFO("MPD protocoll version: %u.%u.%u", mpd_state->protocol[0], mpd_state->protocol[1], mpd_state->protocol[2]);

    // Defaults
    mpd_state->feat_sticker = false;
    mpd_state->feat_playlists = false;
    mpd_state->feat_tags = false;
    mpd_state->feat_advsearch = false;
    mpd_state->feat_fingerprint = false;

    mpd_state->feat_smartpls = mpd_state->smartpls;
    mpd_state->feat_coverimage = true;
    
    if (mpd_send_allowed_commands(mpd_state->conn)) {
        while ((pair = mpd_recv_command_pair(mpd_state->conn)) != NULL) {
            if (strcmp(pair->value, "sticker") == 0) {
                LOG_DEBUG("MPD supports stickers");
                mpd_state->feat_sticker = true;
            }
            else if (strcmp(pair->value, "listplaylists") == 0) {
                LOG_DEBUG("MPD supports playlists");
                mpd_state->feat_playlists = true;
            }
            else if (strcmp(pair->value, "getfingerprint") == 0) {
                LOG_DEBUG("MPD supports fingerprints");
                mpd_state->feat_fingerprint = true;
            }
            mpd_return_pair(mpd_state->conn, pair);
        }
        mpd_response_finish(mpd_state->conn);
    }
    else {
        LOG_ERROR_AND_RECOVER("mpd_send_allowed_commands");
    }
    if (mpd_state->feat_sticker == false && mpd_state->stickers == true) {
        LOG_WARN("MPD don't support stickers, disabling myMPD feature");
        mpd_state->feat_sticker = false;
    }
    if (mpd_state->feat_sticker == true && mpd_state->stickers == false) {
        mpd_state->feat_sticker = false;
    }
    if (mpd_state->feat_sticker == false && mpd_state->smartpls == true) {
        LOG_WARN("Stickers are disabled, disabling smart playlists");
        mpd_state->feat_smartpls = false;
    }
    if (mpd_state->feat_playlists == false && mpd_state->smartpls == true) {
        LOG_WARN("Playlists are disabled, disabling smart playlists");
        mpd_state->feat_smartpls = false;
    }

    mpd_client_feature_music_directory(mpd_state);
    mpd_client_feature_tags(mpd_state);
    mpd_client_feature_love(mpd_state);
    mpd_client_get_state(mpd_state, NULL);
    
    if (LIBMPDCLIENT_CHECK_VERSION(2, 17, 0) && mpd_connection_cmp_server_version(mpd_state->conn, 0, 21, 0) >= 0) {
        mpd_state->feat_advsearch = true;
        LOG_INFO("Enabling advanced search");
    } 
    else {
        LOG_WARN("Disabling advanced search, depends on mpd >= 0.21.0 and libmpdclient >= 2.17.0.");
    }
}

static void mpd_client_idle(t_config *config, t_mpd_state *mpd_state) {
    struct pollfd fds[1];
    int pollrc;
    char buffer[MAX_SIZE];
    size_t len = 0;
    unsigned mpd_client_queue_length = 0;
    enum mpd_idle set_idle_mask = MPD_IDLE_DATABASE | MPD_IDLE_STORED_PLAYLIST | MPD_IDLE_QUEUE | MPD_IDLE_PLAYER | MPD_IDLE_MIXER | \
        MPD_IDLE_OUTPUT | MPD_IDLE_OPTIONS | MPD_IDLE_UPDATE | MPD_IDLE_SUBSCRIPTION;
    
    switch (mpd_state->conn_state) {
        case MPD_WAIT: {
            time_t now = time(NULL);
            if (now > mpd_state->reconnect_time) {
                mpd_state->conn_state = MPD_DISCONNECTED;
            }
            //mpd_client_api error response
            mpd_client_queue_length = tiny_queue_length(mpd_client_queue, 50);
            if (mpd_client_queue_length > 0) {
                //Handle request
                LOG_DEBUG("Handle request (mpd disconnected)");
                t_work_request *request = tiny_queue_shift(mpd_client_queue, 50);
                if (request != NULL) {
                    //create response struct
                    if (request->conn_id > -1) {
                        t_work_result *response = (t_work_result*)malloc(sizeof(t_work_result));
                        assert(response);
                        response->conn_id = request->conn_id;
                        response->length = snprintf(response->data, MAX_SIZE, "{\"type\": \"error\", \"data\": \"MPD disconnected.\"}");
                        LOG_DEBUG("Send http response to connection %lu (first 800 chars):\n%*.*s", request->conn_id, 0, 800, response->data);
                        tiny_queue_push(web_server_queue, response);
                    }
                    FREE_PTR(request);
                }
            }            
            break;
        }
        case MPD_DISCONNECTED:
            /* Try to connect */
            if (strncmp(mpd_state->mpd_host, "/", 1) == 0) {
                LOG_INFO("MPD Connecting to socket %s", mpd_state->mpd_host);
            }
            else {
                LOG_INFO("MPD Connecting to %s:%d", mpd_state->mpd_host, mpd_state->mpd_port);
            }
            mpd_state->conn = mpd_connection_new(mpd_state->mpd_host, mpd_state->mpd_port, mpd_state->timeout);
            if (mpd_state->conn == NULL) {
                printf("ERROR: MPD connection failed.");
                len = snprintf(buffer, MAX_SIZE, "{\"type\": \"mpd_disconnected\"}");
                mpd_client_notify(buffer, len);
                mpd_state->conn_state = MPD_FAILURE;
                mpd_connection_free(mpd_state->conn);
                return;
            }

            if (mpd_connection_get_error(mpd_state->conn) != MPD_ERROR_SUCCESS) {
                LOG_ERROR("MPD connection: %s", mpd_connection_get_error_message(mpd_state->conn));
                len = snprintf(buffer, MAX_SIZE, "{\"type\": \"error\", \"data\": \"MPD connection error: %%{error}\", \"values\": {\"error\": \"%s\"}}", mpd_connection_get_error_message(mpd_state->conn));
                mpd_client_notify(buffer, len);
                mpd_state->conn_state = MPD_FAILURE;
                return;
            }

            if (strlen(mpd_state->mpd_pass) > 0 && !mpd_run_password(mpd_state->conn, mpd_state->mpd_pass)) {
                LOG_ERROR("MPD connection: %s", mpd_connection_get_error_message(mpd_state->conn));
                len = snprintf(buffer, MAX_SIZE, "{\"type\": \"error\", \"data\": \"MPD connection error: %%{error}\", \"values\": {\"error\": \"%s\"}}", mpd_connection_get_error_message(mpd_state->conn));
                mpd_client_notify(buffer, len);
                mpd_state->conn_state = MPD_FAILURE;
                return;
            }

            LOG_INFO("MPD connected");
            mpd_connection_set_timeout(mpd_state->conn, mpd_state->timeout);
            len = snprintf(buffer, MAX_SIZE, "{\"type\": \"mpd_connected\"}");
            mpd_client_notify(buffer, len);
            mpd_state->conn_state = MPD_CONNECTED;
            mpd_state->reconnect_intervall = 0;
            mpd_state->reconnect_time = 0;
            mpd_client_mpd_features(mpd_state);
            mpd_client_smartpls_update_all(config, mpd_state);
            if (mpd_state->jukebox_mode != JUKEBOX_OFF) {
                mpd_client_jukebox(mpd_state);
            }
            if (!mpd_send_idle_mask(mpd_state->conn, set_idle_mask)) {
                LOG_ERROR("Entering idle mode failed");
                mpd_state->conn_state = MPD_FAILURE;
            }
            break;

        case MPD_FAILURE:
            LOG_ERROR("MPD connection failed");
            len = snprintf(buffer, MAX_SIZE, "{\"type\": \"mpd_disconnected\"}");
            mpd_client_notify(buffer, len);
            // fall through
        case MPD_DISCONNECT:
        case MPD_RECONNECT:
            if (mpd_state->conn != NULL) {
                mpd_connection_free(mpd_state->conn);
            }
            mpd_state->conn = NULL;
            mpd_state->conn_state = MPD_WAIT;
            if (mpd_state->reconnect_intervall <= 20) {
                mpd_state->reconnect_intervall += 2;
            }
            mpd_state->reconnect_time = time(NULL) + mpd_state->reconnect_intervall;
            LOG_DEBUG("Waiting %u seconds before reconnection", mpd_state->reconnect_intervall);
            break;

        case MPD_CONNECTED:
            fds[0].fd = mpd_connection_get_fd(mpd_state->conn);
            fds[0].events = POLLIN;
            pollrc = poll(fds, 1, 50);
            bool jukebox_add_song = false;
            bool set_played = false;
            mpd_client_queue_length = tiny_queue_length(mpd_client_queue, 50);
            time_t now = time(NULL);
            if (now > mpd_state->set_song_played_time && mpd_state->set_song_played_time > 0 && mpd_state->last_last_played_id != mpd_state->song_id) {
                set_played = true;
            }
            if (mpd_state->jukebox_mode != JUKEBOX_OFF) {
                time_t add_time = mpd_state->crossfade < mpd_state->song_end_time ? mpd_state->song_end_time - mpd_state->crossfade : mpd_state->song_end_time;
                if (now > add_time && add_time > 0 && mpd_state->queue_length <= mpd_state->jukebox_queue_length) {
                    jukebox_add_song = true;
                }
            }
            if (pollrc > 0 || mpd_client_queue_length > 0 || jukebox_add_song == true || set_played == true) {
                LOG_DEBUG("Leaving mpd idle mode");
                if (!mpd_send_noidle(mpd_state->conn)) {
                    LOG_ERROR("Leaving idle mode failed");
                    mpd_state->conn_state = MPD_FAILURE;
                    break;
                }
                if (pollrc > 0) {
                    //Handle idle events
                    LOG_DEBUG("Checking for idle events");
                    enum mpd_idle idle_bitmask = mpd_recv_idle(mpd_state->conn, false);
                    mpd_client_parse_idle(config, mpd_state, idle_bitmask);
                } 
                else {
                    mpd_response_finish(mpd_state->conn);
                }
                
                if (set_played == true) {
                    mpd_client_last_played_list(config, mpd_state, mpd_state->song_id);
                    if (mpd_state->feat_sticker == true) {
                        mpd_client_count_song_uri(mpd_state, mpd_state->song_uri, "playCount", 1);
                        mpd_client_last_played_song_uri(mpd_state, mpd_state->song_uri);
                    }
                }
                
                if (jukebox_add_song == true) {
                    mpd_client_jukebox(mpd_state);
                }
                
                if (mpd_client_queue_length > 0) {
                    //Handle request
                    LOG_DEBUG("Handle request");
                    t_work_request *request = tiny_queue_shift(mpd_client_queue, 50);
                    if (request != NULL) {
                        mpd_client_api(config, mpd_state, request);
                    }
                }
                LOG_DEBUG("Entering mpd idle mode");
                if (!mpd_send_idle_mask(mpd_state->conn, set_idle_mask)) {
                    LOG_ERROR("Entering idle mode failed");
                    mpd_state->conn_state = MPD_FAILURE;
                }
            }
            break;
        default:
            LOG_ERROR("Invalid mpd connection state");
    }
}

static int mpd_client_get_updatedb_state(t_mpd_state *mpd_state, char *buffer) {
    struct mpd_status *status;
    int len, update_id;
    struct json_out out = JSON_OUT_BUF(buffer, MAX_SIZE);

    status = mpd_run_status(mpd_state->conn);
    if (!status) {
        RETURN_ERROR_AND_RECOVER("mpd_run_status");
    }
    update_id = mpd_status_get_update_id(status);
    LOG_INFO("Update database ID: %d", update_id);
    if ( update_id > 0) {
        len = json_printf(&out, "{type: update_started, data: {jobid: %d}}", update_id);
    }
    else {
        len = json_printf(&out, "{type: update_finished}");
    }
    mpd_status_free(status);    
    
    CHECK_RETURN_LEN();
}


static bool mpd_client_get_sticker(t_mpd_state *mpd_state, const char *uri, t_sticker *sticker) {
    struct mpd_pair *pair;
    char *crap = NULL;
    sticker->playCount = 0;
    sticker->skipCount = 0;
    sticker->lastPlayed = 0;
    sticker->lastSkipped = 0;
    sticker->like = 1;
    
    if (uri == NULL || strstr(uri, "://") != NULL) {
        return false;
    }

    if (mpd_send_sticker_list(mpd_state->conn, "song", uri)) {
        while ((pair = mpd_recv_sticker(mpd_state->conn)) != NULL) {
            if (strcmp(pair->name, "playCount") == 0) {
                sticker->playCount = strtoimax(pair->value, &crap, 10);
            }
            else if (strcmp(pair->name, "skipCount") == 0) {
                sticker->skipCount = strtoimax(pair->value, &crap, 10);
            }
            else if (strcmp(pair->name, "lastPlayed") == 0) {
                sticker->lastPlayed = strtoimax(pair->value, &crap, 10);
            }
            else if (strcmp(pair->name, "lastSkipped") == 0) {
                sticker->lastSkipped = strtoimax(pair->value, &crap, 10);
            }
            else if (strcmp(pair->name, "like") == 0) {
                sticker->like = strtoimax(pair->value, &crap, 10);
            }
            mpd_return_sticker(mpd_state->conn, pair);
        }
    }
    else {
        LOG_ERROR_AND_RECOVER("mpd_send_sticker_list");
        return false;
    }
    return true;
}

static bool mpd_client_count_song_uri(t_mpd_state *mpd_state, const char *uri, const char *name, const int value) {
    if (uri == NULL || strstr(uri, "://") != NULL) {
        return false;
    }
    struct mpd_pair *pair;
    char *crap = NULL;
    int old_value = 0;
    char v[10];
    
    if (mpd_send_sticker_list(mpd_state->conn, "song", uri)) {
        while ((pair = mpd_recv_sticker(mpd_state->conn)) != NULL) {
            if (strcmp(pair->name, name) == 0) {
                old_value = strtoimax(pair->value, &crap, 10);
            }
            mpd_return_sticker(mpd_state->conn, pair);
        }
    } else {
        LOG_ERROR_AND_RECOVER("mpd_send_sticker_list");
        return false;
    }
    old_value += value;
    if (old_value > 999999999) {
        old_value = 999999999;
    }
    else if (old_value < 0) {
        old_value = 0;
    }
    snprintf(v, 10, "%d", old_value);
    LOG_VERBOSE("Setting sticker: \"%s\" -> %s: %s", uri, name, v);
    if (!mpd_run_sticker_set(mpd_state->conn, "song", uri, name, v)) {
        LOG_ERROR_AND_RECOVER("mpd_send_sticker_set");
        return false;
    }
    return true;
}

static bool mpd_client_like_song_uri(t_mpd_state *mpd_state, const char *uri, int value) {
    if (uri == NULL || strstr(uri, "://") != NULL) {
        return false;
    }
    char v[10];
    if (value > 2) {
        value = 2;
    }
    else if (value < 0) {
        value = 0;
    }
    snprintf(v, 10, "%d", value);
    LOG_VERBOSE("Setting sticker: \"%s\" -> like: %s", uri, v);
    if (!mpd_run_sticker_set(mpd_state->conn, "song", uri, "like", v)) {
        LOG_ERROR_AND_RECOVER("mpd_send_sticker_set");
        return false;
    }
    return true;        
}

static bool mpd_client_last_played_list_save(t_config *config, t_mpd_state *mpd_state) {
    size_t tmp_file_len = config->varlibdir_len + 26;
    char tmp_file[tmp_file_len];
    size_t cfg_file_len = config->varlibdir_len + 19;
    char cfg_file[cfg_file_len];
    snprintf(cfg_file, cfg_file_len, "%s/state/last_played", config->varlibdir);
    snprintf(tmp_file, tmp_file_len, "%s/state/last_played.XXXXXX", config->varlibdir);
    int fd;
    int i = 0;

    LOG_VERBOSE("Saving last_played list to disc");

    if ((fd = mkstemp(tmp_file)) < 0 ) {
        LOG_ERROR("Can't open %s for write", tmp_file);
        return false;
    }    
    
    FILE *fp = fdopen(fd, "w");
    //first write last_played list to tmp file
    struct node *current = mpd_state->last_played.list;
    while (current != NULL && i < mpd_state->last_played_count) {
        fprintf(fp, "%d::%s\n", current->value, current->data);
        current = current->next;
        i++;
    }
    //append current last_played file to tmp file
    char *line = NULL;
    size_t n = 0;
    ssize_t read;
    FILE *fi = fopen(cfg_file, "r");
    if (fi != NULL) {
        while ((read = getline(&line, &n, fi)) > 0 && i < mpd_state->last_played_count) {
            fprintf(fp, "%s", line);
            i++;
        }
        FREE_PTR(line);
        fclose(fi);
    }
    fclose(fp);
    if (rename(tmp_file, cfg_file) == -1) {
        LOG_ERROR("Renaming file from %s to %s failed", tmp_file, cfg_file);
        return false;
    }
    //empt list after write to disc
    list_free(&mpd_state->last_played);    
    return true;
}

static bool mpd_client_last_played_list(t_config *config, t_mpd_state *mpd_state, const int song_id) {
    struct mpd_song *song;

    if (song_id > -1) {
        song = mpd_run_get_queue_song_id(mpd_state->conn, song_id);
        if (song) {
            const char *uri = mpd_song_get_uri(song);
            if (uri == NULL || strstr(uri, "://") != NULL) {
                //Don't add streams to last played list
                mpd_state->last_last_played_id = song_id;
                mpd_song_free(song);
                return true;
            }
            else {
                list_insert(&mpd_state->last_played, uri, time(NULL));
            }
            mpd_state->last_last_played_id = song_id;
            mpd_song_free(song);
            //write last_played list to disc
            if (mpd_state->last_played.length > 9 || mpd_state->last_played.length > mpd_state->last_played_count) {
                mpd_client_last_played_list_save(config, mpd_state);
            }
            //notify clients
            char buffer[MAX_SIZE];
            size_t len = snprintf(buffer, MAX_SIZE, "{\"type\": \"update_lastplayed\"}");
            mpd_client_notify(buffer, len);
        } else {
            LOG_ERROR("Can't get song from id %d", song_id);
            return false;
        }
    }
    return true;
}

static bool mpd_client_last_played_song_uri(t_mpd_state *mpd_state, const char *uri) {
    if (uri == NULL || strstr(uri, "://") != NULL) {
        return false;
    }
    char v[20];
    snprintf(v, 20, "%ld", mpd_state->song_start_time);
    LOG_VERBOSE("Setting sticker: \"%s\" -> lastPlayed: %s", uri, v);
    if (!mpd_run_sticker_set(mpd_state->conn, "song", uri, "lastPlayed", v)) {
        LOG_ERROR_AND_RECOVER("mpd_run_sticker_set");
        return false;
    }
    return true;
}

static bool mpd_client_last_skipped_song_uri(t_mpd_state *mpd_state, const char *uri) {
    if (uri == NULL || strstr(uri, "://") != NULL) {
        return false;
    }
    char v[20];
    time_t now = time(NULL);
    snprintf(v, 20, "%ld", now);
    LOG_VERBOSE("Setting sticker: \"%s\" -> lastSkipped: %s", uri, v);
    if (!mpd_run_sticker_set(mpd_state->conn, "song", uri, "lastSkipped", v)) {
        LOG_ERROR_AND_RECOVER("mpd_run_sticker_set");
        return false;
    }
    return true;
}

static char *mpd_client_get_tag(struct mpd_song const *song, const enum mpd_tag_type tag) {
    char *str = (char *)mpd_song_get_tag(song, tag, 0);
    if (str == NULL) {
        if (tag == MPD_TAG_TITLE) {
            str = basename((char *)mpd_song_get_uri(song));
        }
        else if (tag == MPD_TAG_ALBUM_ARTIST) {
            str = (char *)mpd_song_get_tag(song, MPD_TAG_ARTIST, 0);
        }
    }
    return str;
}

static bool mpd_client_jukebox(t_mpd_state *mpd_state) {
    size_t queue_length;
    int addSongs;
    
    struct mpd_status *status = mpd_run_status(mpd_state->conn);
    if (!status) {
        LOG_ERROR_AND_RECOVER("mpd_run_status");
        return false;
    }
    queue_length = mpd_status_get_queue_length(status);
    mpd_status_free(status);
    
    time_t now = time(NULL);
    time_t add_time = mpd_state->crossfade < mpd_state->song_end_time ? mpd_state->song_end_time - mpd_state->crossfade : 0;
    
    if (queue_length >= mpd_state->jukebox_queue_length && now < add_time) {
        LOG_DEBUG("Jukebox: Queue length >= %d and add_time not reached", mpd_state->jukebox_queue_length);
        return true;
    }

    //add song if add_time is reached or queue is empty
    addSongs = mpd_state->jukebox_queue_length - queue_length;
    if (now > add_time && add_time > 0) {
        addSongs++;
    }

    if (addSongs < 1) {
        return true;
    }
        
    if (mpd_state->feat_playlists == false && strcmp(mpd_state->jukebox_playlist, "Database") != 0) {
        LOG_WARN("Jukebox: Playlists are disabled");
        return true;
    }

    bool rc = mpd_client_jukebox_add(mpd_state, addSongs, mpd_state->jukebox_mode, mpd_state->jukebox_playlist);
    
    if (rc == true) {
        if (!mpd_run_play(mpd_state->conn)) {
            LOG_ERROR_AND_RECOVER("mpd_run_play");
        }
    }
    else {
        LOG_ERROR("Error adding song(s), trying again");
        mpd_client_jukebox(mpd_state);
    }
    return rc;
}

static bool mpd_client_jukebox_add(t_mpd_state *mpd_state, const int addSongs, const enum jukebox_modes jukebox_mode, const char *jukebox_playlist) {
    int i;
    struct mpd_entity *entity;
    const struct mpd_song *song;
    struct mpd_pair *pair;
    int lineno = 1;
    int nkeep = 0;
    
    if (addSongs < 1) {
        return false;
    }
        
    struct list add_list;
    list_init(&add_list);
    
    if (jukebox_mode == JUKEBOX_ADD_SONG) {
        //add songs
        if (strcmp(jukebox_playlist, "Database") == 0) {
            if (!mpd_send_list_all(mpd_state->conn, "/")) {
                LOG_ERROR_AND_RECOVER("mpd_send_list_all");
                list_free(&add_list);
                return false;
            }
        }
        else {
            if (!mpd_send_list_playlist(mpd_state->conn, jukebox_playlist)) {
                LOG_ERROR_AND_RECOVER("mpd_send_list_playlist");
                list_free(&add_list);
                return false;
            }
        }
        while ((entity = mpd_recv_entity(mpd_state->conn)) != NULL) {
            if (mpd_entity_get_type(entity) == MPD_ENTITY_TYPE_SONG) {
                if (randrange(lineno) < addSongs) {
		    if (nkeep < addSongs) {
		        song = mpd_entity_get_song(entity);
		        list_push(&add_list, mpd_song_get_uri(song), lineno);
		        nkeep++;
                    }
                    else {
                        i = addSongs > 1 ? randrange(addSongs) : 0;
                        song = mpd_entity_get_song(entity);
                        list_replace(&add_list, i, mpd_song_get_uri(song), lineno);
                    }
                }
                lineno++;
            }
            mpd_entity_free(entity);
        }
    }
    else if (jukebox_mode == JUKEBOX_ADD_ALBUM) {
        //add album
        if (!mpd_search_db_tags(mpd_state->conn, MPD_TAG_ALBUM)) {
            LOG_ERROR_AND_RECOVER("mpd_search_db_tags");
            list_free(&add_list);
            return false;
        }
        if (!mpd_search_commit(mpd_state->conn)) {
            LOG_ERROR_AND_RECOVER("mpd_search_commit");
            list_free(&add_list);
            return false;
        }
        while ((pair = mpd_recv_pair_tag(mpd_state->conn, MPD_TAG_ALBUM )) != NULL)  {
            if (randrange(lineno) < addSongs) {
		if (nkeep < addSongs) {
                    list_push(&add_list, strdup(pair->value), lineno);
                    nkeep++;
                }
		else {
                    i = addSongs > 1 ? randrange(addSongs) : 0;
                    list_replace(&add_list, i, strdup(pair->value), lineno);
                }
            }
            lineno++;
            mpd_return_pair(mpd_state->conn, pair);
        }
    }

    if (nkeep < addSongs) {
        LOG_WARN("Input didn't contain %d entries", addSongs);
    }

    list_shuffle(&add_list);

    nkeep = 0;
    struct node *current = add_list.list;
    while (current != NULL) {
        if (jukebox_mode == JUKEBOX_ADD_SONG) {
	    LOG_INFO("Jukebox adding song: %s", current->data);
	    if (!mpd_run_add(mpd_state->conn, current->data)) {
                LOG_ERROR_AND_RECOVER("mpd_run_add");
            }
            else {
                nkeep++;
            }
        }
        else {
            LOG_INFO("Jukebox adding album: %s", current->data);
            if (!mpd_send_command(mpd_state->conn, "searchadd", "Album", current->data, NULL)) {
                LOG_ERROR_AND_RECOVER("mpd_send_command");
                return false;
            }
            else {
                nkeep++;
            }
            mpd_response_finish(mpd_state->conn);
        }
        current = current->next;
    }
    list_free(&add_list);
    if (nkeep > 0) {
        if (!mpd_run_play(mpd_state->conn)) {
            LOG_ERROR_AND_RECOVER("mpd_run_play");
        }
    }
    else {
        LOG_ERROR("Error adding song(s)");
        return false;
    }
    return true;
}

static int mpd_client_get_state(t_mpd_state *mpd_state, char *buffer) {
    int len = 0;
    struct mpd_status *status = mpd_run_status(mpd_state->conn);
    if (!status) {
        LOG_ERROR_AND_RECOVER("mpd_run_status");
        return -1;
    }

    int song_id = mpd_status_get_song_id(status);
    if (mpd_state->song_id != song_id) {
        mpd_state->last_song_id = mpd_state->song_id;
        mpd_state->last_song_end_time = mpd_state->song_end_time;
        mpd_state->last_song_start_time = mpd_state->song_start_time;
        struct mpd_song *song = mpd_run_current_song(mpd_state->conn);
        if (song != NULL) {
            FREE_PTR(mpd_state->last_song_uri);
            if (mpd_state->song_uri != NULL) {
                mpd_state->last_song_uri = mpd_state->song_uri;
            }
            mpd_state->song_uri = strdup(mpd_song_get_uri(song));
            mpd_song_free(song);
        }
        else {
            FREE_PTR(mpd_state->song_uri);
        }
        mpd_response_finish(mpd_state->conn);
    }

    mpd_state->state = mpd_status_get_state(status);
    mpd_state->song_id = song_id;
    mpd_state->next_song_id = mpd_status_get_next_song_id(status);
    mpd_state->queue_version = mpd_status_get_queue_version(status);
    mpd_state->queue_length = mpd_status_get_queue_length(status);
    mpd_state->crossfade = mpd_status_get_crossfade(status);

    const int total_time = mpd_status_get_total_time(status);
    const int elapsed_time =  mpd_status_get_elapsed_time(status);
    if (total_time > 10) {
        time_t now = time(NULL);
        mpd_state->song_end_time = now + total_time - elapsed_time - 10;
        mpd_state->song_start_time = now - elapsed_time;
        int half_time = total_time / 2;
        
        if (half_time > 240) {
            mpd_state->set_song_played_time = now - elapsed_time + 240;
        }
        else {
            mpd_state->set_song_played_time = elapsed_time < half_time ? now - elapsed_time + half_time : now;
        }
    }
    else {
        //don't track songs with length < 10s
        mpd_state->song_end_time = 0;
        mpd_state->song_start_time = 0;
        mpd_state->set_song_played_time = 0;
    }
    
    if (buffer != NULL) {
        len = mpd_client_put_state(mpd_state, status, buffer);
    }
    mpd_status_free(status);
    return len;
}

static int mpd_client_put_state(t_mpd_state *mpd_state, struct mpd_status *status, char *buffer) {
    int len = 0;
    struct json_out out = JSON_OUT_BUF(buffer, MAX_SIZE);

    const struct mpd_audio_format *audioformat = mpd_status_get_audio_format(status);

    len = json_printf(&out, "{type: update_state, data: {state: %d, volume: %d, songPos: %d, elapsedTime: %d, "
        "totalTime: %d, currentSongId: %d, kbitrate: %d, audioFormat: { sampleRate: %d, bits: %d, channels: %d}, "
        "queueLength: %d, nextSongPos: %d, nextSongId: %d, lastSongId: %d, queueVersion: %d}}", 
        mpd_status_get_state(status),
        mpd_status_get_volume(status), 
        mpd_status_get_song_pos(status),
        mpd_status_get_elapsed_time(status),
        mpd_status_get_total_time(status),
        mpd_status_get_song_id(status),
        mpd_status_get_kbit_rate(status),
        audioformat ? audioformat->sample_rate : 0, 
        audioformat ? audioformat->bits : 0, 
        audioformat ? audioformat->channels : 0,
        mpd_status_get_queue_length(status),
        mpd_status_get_next_song_pos(status),
        mpd_status_get_next_song_id(status),
        mpd_state->last_song_id ? mpd_state->last_song_id : -1,
        mpd_status_get_queue_version(status)
    );
   
    CHECK_RETURN_LEN();
}

static int mpd_client_get_queue_state(t_mpd_state *mpd_state, char *buffer) {
    int len = 0;
    struct mpd_status *status = mpd_run_status(mpd_state->conn);
    if (!status) {
        LOG_ERROR_AND_RECOVER("mpd_run_status");
        return -1;
    }

    mpd_state->queue_version = mpd_status_get_queue_version(status);
    mpd_state->queue_length = mpd_status_get_queue_length(status);
    mpd_state->crossfade = mpd_status_get_crossfade(status);
    mpd_state->state = mpd_status_get_state(status);

    if (buffer != NULL) {
        len = mpd_client_put_queue_state(status, buffer);
    }
    mpd_status_free(status);
    return len;
}

static int mpd_client_put_queue_state(struct mpd_status *status, char *buffer) {
    size_t len = 0;
    struct json_out out = JSON_OUT_BUF(buffer, MAX_SIZE);

    len = json_printf(&out, "{type: update_queue, data: {state: %d, queueLength: %d, queueVersion: %d, songPos: %d, nextSongPos: %d}}", 
        mpd_status_get_state(status),
        mpd_status_get_queue_length(status),
        mpd_status_get_queue_version(status),
        mpd_status_get_song_pos(status),
        mpd_status_get_next_song_pos(status)
    );
   
    CHECK_RETURN_LEN();
}

static int mpd_client_put_volume(t_mpd_state *mpd_state, char *buffer) {
    size_t len = 0;
    struct json_out out = JSON_OUT_BUF(buffer, MAX_SIZE);

    struct mpd_status *status = mpd_run_status(mpd_state->conn);
    if (!status) {
        RETURN_ERROR_AND_RECOVER("mpd_run_status");
    }
    len = json_printf(&out, "{type: update_volume, data: {volume: %d}}",
        mpd_status_get_volume(status)
    );
    mpd_status_free(status);

    CHECK_RETURN_LEN();
}

static int mpd_client_put_settings(t_mpd_state *mpd_state, char *buffer) {
    char *replaygain = NULL;
    size_t len, nr;
    struct json_out out = JSON_OUT_BUF(buffer, MAX_SIZE);
    
    struct mpd_status *status = mpd_run_status(mpd_state->conn);
    if (!status) {
        RETURN_ERROR_AND_RECOVER("mpd_run_status");
    }

    if (!mpd_send_command(mpd_state->conn, "replay_gain_status", NULL)) {
        RETURN_ERROR_AND_RECOVER("replay_gain_status");
    }
    struct mpd_pair *pair = mpd_recv_pair(mpd_state->conn);
    if (!pair) {
        RETURN_ERROR_AND_RECOVER("replay_gain_status");
    }
    replaygain = strdup(pair->value);
    mpd_return_pair(mpd_state->conn, pair);
    mpd_response_finish(mpd_state->conn);
    
    len = json_printf(&out, "{type: settings, data: {"
        "repeat: %d, single: %d, crossfade: %d, consume: %d, random: %d, "
        "mixrampdb: %f, mixrampdelay: %f, replaygain: %Q, featPlaylists: %B,"
        "featTags: %B, featLibrary: %B, featAdvsearch: %B, featStickers: %B,"
        "featSmartpls: %B, featLove: %B, featCoverimage: %B, featFingerprint: %B, "
        "musicDirectoryValue: %Q, mpdConnected: %B, tags: [", 
        mpd_status_get_repeat(status),
        mpd_status_get_single(status),
        mpd_status_get_crossfade(status),
        mpd_status_get_consume(status),
        mpd_status_get_random(status),
        mpd_status_get_mixrampdb(status),
        mpd_status_get_mixrampdelay(status),
        replaygain == NULL ? "" : replaygain,
        mpd_state->feat_playlists,
        mpd_state->feat_tags,
        mpd_state->feat_library,
        mpd_state->feat_advsearch,
        mpd_state->feat_sticker,
        mpd_state->feat_smartpls,
        mpd_state->feat_love,
        mpd_state->feat_coverimage,
        mpd_state->feat_fingerprint,
        mpd_state->music_directory_value,
        true
    );
    mpd_status_free(status);
    FREE_PTR(replaygain);
    
    for (nr = 0; nr < mpd_state->mympd_tag_types_len; nr++) {
        if (nr > 0) 
            len += json_printf(&out, ",");
        len += json_printf(&out, "%Q", mpd_tag_name(mpd_state->mympd_tag_types[nr]));
    }
    
    len += json_printf(&out, "], searchtags: [");
        for (nr = 0; nr < mpd_state->search_tag_types_len; nr++) {
        if (nr > 0) 
            len += json_printf(&out, ",");
        len += json_printf(&out, "%Q", mpd_tag_name(mpd_state->search_tag_types[nr]));
    }
    
    len += json_printf(&out, "], browsetags: [");
    for (nr = 0; nr < mpd_state->browse_tag_types_len; nr++) {
        if (nr > 0) 
            len += json_printf(&out, ",");
        len += json_printf(&out, "%Q", mpd_tag_name(mpd_state->browse_tag_types[nr]));
    }

    len += json_printf(&out, "], allmpdtags: [");
    for (nr = 0; nr < mpd_state->mpd_tag_types_len; nr++) {
        if (nr > 0) 
            len += json_printf(&out, ",");
        len += json_printf(&out, "%Q", mpd_tag_name(mpd_state->mpd_tag_types[nr]));
    }

    len += json_printf(&out, "]}}");
    
    CHECK_RETURN_LEN();
}

static int mpd_client_put_outputs(t_mpd_state *mpd_state, char *buffer) {
    struct mpd_output *output;
    size_t len;
    int nr;
    struct json_out out = JSON_OUT_BUF(buffer, MAX_SIZE);
    
    if (!mpd_send_outputs(mpd_state->conn)) {
        RETURN_ERROR_AND_RECOVER("outputs");
    }

    len = json_printf(&out, "{type: outputs, data: {outputs: [");
    nr = 0;    
    while ((output = mpd_recv_output(mpd_state->conn)) != NULL && len < MAX_LIST_SIZE) {
        if (nr++) 
            len += json_printf(&out, ",");
        len += json_printf(&out, "{id: %d, name: %Q, state: %d}",
            mpd_output_get_id(output),
            mpd_output_get_name(output),
            mpd_output_get_enabled(output)
        );
        mpd_output_free(output);
    }
    if (!mpd_response_finish(mpd_state->conn))
        LOG_ERROR_AND_RECOVER("outputs");

    len += json_printf(&out,"]}}");
    
    CHECK_RETURN_LEN();
}

static int mpd_client_get_cover(t_config *config, t_mpd_state *mpd_state, const char *uri, char *cover, const int cover_len) {
    char *orgpath = strdup(uri);
    char *path = orgpath;
    size_t len = 0;

    if (mpd_state->feat_coverimage == false) {
        len = snprintf(cover, cover_len, "/assets/coverimage-notavailable.svg");
    }
    else if (strstr(path, "://") != NULL) {
        char *name = strstr(path, "://");
        name += 3;
        replacechar(name, '/', '_');
        replacechar(name, '.', '_');
        replacechar(name, ':', '_');
        snprintf(cover, cover_len, "%s/pics/%s.png", config->varlibdir, name);
        LOG_DEBUG("Check for cover %s", cover);
        if (access(cover, F_OK ) == -1 ) {
            len = snprintf(cover, cover_len, "/assets/coverimage-stream.svg");
        }
        else {
            len = snprintf(cover, cover_len, "/pics/%s.png", name);
        }
    }
    else {
        if (mpd_state->feat_library == true && strlen(mpd_state->music_directory_value) > 0) {
            dirname(path);
            snprintf(cover, cover_len, "%s/%s/%s", mpd_state->music_directory_value, path, mpd_state->coverimage_name);
            if (access(cover, F_OK ) == -1 ) {
                if (config->plugins_coverextract == true) {
                    size_t media_file_len = strlen(mpd_state->music_directory_value) + strlen(uri) + 2;
                    char media_file[media_file_len];
                    snprintf(media_file, media_file_len, "%s/%s", mpd_state->music_directory_value, uri);
                    size_t image_file_len = 1500;
                    char image_file[image_file_len];
                    size_t image_mime_type_len = 100;
                    char image_mime_type[image_mime_type_len];
                    bool rc = plugin_coverextract(media_file, "", image_file, image_file_len, image_mime_type, image_mime_type_len, false);
                    if (rc == true) {
                        len = snprintf(cover, cover_len, "/library/%s?cover", uri);
                    }
                    else {
                        len = snprintf(cover, cover_len, "/assets/coverimage-notavailable.svg");
                    }
                }
                else {
                    len = snprintf(cover, cover_len, "/assets/coverimage-notavailable.svg");
                }
            }
            else {
                len = snprintf(cover, cover_len, "/library/%s/%s", path, mpd_state->coverimage_name);
            }
        } else {
            len = snprintf(cover, cover_len, "/assets/coverimage-notavailable.svg");
        }
    }
    FREE_PTR(orgpath);
    return len;
}

static int mpd_client_put_current_song(t_config *config, t_mpd_state *mpd_state, char *buffer) {
    size_t len = 0;
    struct json_out out = JSON_OUT_BUF(buffer, MAX_SIZE);
    size_t cover_len = 2000;
    char cover[cover_len];
    
    struct mpd_song *song = mpd_run_current_song(mpd_state->conn);
    if (song == NULL) {
        len = json_printf(&out, "{type: result, data: ok}");
        return len;
    }
        
    mpd_client_get_cover(config, mpd_state, mpd_song_get_uri(song), cover, cover_len);
    
    len = json_printf(&out, "{type: song_change, data: {pos: %d, currentSongId: %d, cover: %Q, ",
        mpd_song_get_pos(song),
        mpd_state->song_id,
        cover
    );
    PUT_SONG_TAG_ALL();

    mpd_response_finish(mpd_state->conn);
    
    if (mpd_state->feat_sticker) {
        t_sticker *sticker = (t_sticker *) malloc(sizeof(t_sticker));
        assert(sticker);
        mpd_client_get_sticker(mpd_state, mpd_song_get_uri(song), sticker);
        len += json_printf(&out, ", playCount: %d, skipCount: %d, like: %d, lastPlayed: %d, lastSkipped: %d",
            sticker->playCount,
            sticker->skipCount,
            sticker->like,
            sticker->lastPlayed,
            sticker->lastSkipped
        );
        FREE_PTR(sticker);
    }
    len += json_printf(&out, "}}");
    mpd_song_free(song);

    CHECK_RETURN_LEN();
}

static int mpd_client_put_fingerprint(t_mpd_state *mpd_state, char *buffer, const char *uri) {
    size_t len = 0;
    struct json_out out = JSON_OUT_BUF(buffer, MAX_SIZE);
    
    len = json_printf(&out, "{type: fingerprint, data: {");
    if (mpd_state->feat_fingerprint == true) {
        char fp_buffer[8192];
        const char *fingerprint = mpd_run_getfingerprint_chromaprint(mpd_state->conn, uri, fp_buffer, sizeof(fp_buffer));
        if (fingerprint == NULL) {
            RETURN_ERROR_AND_RECOVER("mpd_getfingerprint");
        }
        len += json_printf(&out, "fingerprint: %Q", fingerprint);
        mpd_response_finish(mpd_state->conn);
    }
    else {
        len += json_printf(&out, "fingerprint: %Q", "not supported by mpd");
    }

    len += json_printf(&out, "}}");
    
    CHECK_RETURN_LEN();
}

static int mpd_client_put_songdetails(t_config *config, t_mpd_state *mpd_state, char *buffer, const char *uri) {
    struct mpd_entity *entity;
    const struct mpd_song *song;
    size_t len = 0;
    struct json_out out = JSON_OUT_BUF(buffer, MAX_SIZE);
    size_t cover_len = 2000;
    char cover[2000] = "";
    
    len = json_printf(&out, "{type: song_details, data: {");

    if (!mpd_send_list_all_meta(mpd_state->conn, uri)) {
        RETURN_ERROR_AND_RECOVER("mpd_send_list_all_meta");
    }
    if ((entity = mpd_recv_entity(mpd_state->conn)) != NULL) {
        song = mpd_entity_get_song(entity);
        mpd_client_get_cover(config, mpd_state, uri, cover, cover_len);
        len += json_printf(&out, "cover: %Q, ", cover);
        PUT_SONG_TAG_ALL();
        mpd_entity_free(entity);
    }
    mpd_response_finish(mpd_state->conn);

    if (mpd_state->feat_sticker) {
        t_sticker *sticker = (t_sticker *) malloc(sizeof(t_sticker));
        assert(sticker);
        mpd_client_get_sticker(mpd_state, uri, sticker);
        len += json_printf(&out, ", playCount: %d, skipCount: %d, like: %d, lastPlayed: %d, lastSkipped: %d",
            sticker->playCount,
            sticker->skipCount,
            sticker->like,
            sticker->lastPlayed,
            sticker->lastSkipped
        );
        FREE_PTR(sticker);
    }
    len += json_printf(&out, "}}");
    
    CHECK_RETURN_LEN();
}

static int mpd_client_put_last_played_songs(t_config *config, t_mpd_state *mpd_state, char *buffer, unsigned int offset, const t_tags *tagcols) {
    const struct mpd_song *song;
    struct mpd_entity *entity;
    size_t len = 0;
    unsigned entity_count = 0;
    unsigned entities_returned = 0;
    struct json_out out = JSON_OUT_BUF(buffer, MAX_SIZE);
    
    len = json_printf(&out, "{type: last_played_songs, data: [");
    
    struct node *current = mpd_state->last_played.list;
    while (current != NULL && len < MAX_LIST_SIZE) {
        entity_count++;
        if (entity_count > offset && entity_count <= offset + mpd_state->max_elements_per_page) {
            if (entities_returned++) 
                len += json_printf(&out, ",");
            len += json_printf(&out, "{Pos: %d, LastPlayed: %d, ", entity_count, current->value);
            if (!mpd_send_list_all_meta(mpd_state->conn, current->data))
                RETURN_ERROR_AND_RECOVER("mpd_send_list_all_meta");
            if ((entity = mpd_recv_entity(mpd_state->conn)) != NULL) {
                song = mpd_entity_get_song(entity);
                PUT_SONG_TAG_COLS(tagcols);
                mpd_entity_free(entity);
                mpd_response_finish(mpd_state->conn);
            }
            len += json_printf(&out, "}");
        }
        current = current->next;
    }

    char *line = NULL;
    char *data = NULL;
    char *crap = NULL;
    size_t n = 0;
    ssize_t read;
    
    size_t lp_file_len = config->varlibdir_len + 19;
    char lp_file[lp_file_len];
    snprintf(lp_file, lp_file_len, "%s/state/last_played", config->varlibdir);
    FILE *fp = fopen(lp_file, "r");
    if (fp != NULL) {
        while ((read = getline(&line, &n, fp)) > 0 && entity_count > offset && entity_count <= offset + mpd_state->max_elements_per_page) {
            int value = strtoimax(line, &data, 10);
            if (strlen(data) > 2) {
                data = data + 2;
                strtok_r(data, "\n", &crap);
                if (entities_returned++) 
                    len += json_printf(&out, ",");
                len += json_printf(&out, "{Pos: %d, LastPlayed: %d, ", entity_count, value);
                if (!mpd_send_list_all_meta(mpd_state->conn, data)) {
                    LOG_ERROR_AND_RECOVER("mpd_send_list_all_meta");
                }
                if ((entity = mpd_recv_entity(mpd_state->conn)) != NULL) {
                    song = mpd_entity_get_song(entity);
                    PUT_SONG_TAG_COLS(tagcols);
                    mpd_entity_free(entity);
                    mpd_response_finish(mpd_state->conn);
                }
                len += json_printf(&out, "}");
            }
            else {
                LOG_ERROR("Reading last_played line failed");
            }
        }
        fclose(fp);
        FREE_PTR(line);
    }

    len += json_printf(&out, "], totalEntities: %d, offset: %d, returnedEntities: %d}",
        entity_count,
        offset,
        entities_returned
    );

    CHECK_RETURN_LEN();
}

static int mpd_client_put_queue(t_mpd_state *mpd_state, char *buffer, const unsigned int offset, const t_tags *tagcols) {
    struct mpd_entity *entity;
    int totalTime = 0;
    unsigned entity_count = 0;
    unsigned entities_returned = 0;
    size_t len;
    struct json_out out = JSON_OUT_BUF(buffer, MAX_SIZE);
    struct mpd_status *status = mpd_run_status(mpd_state->conn);
    
    if (!status) {
        RETURN_ERROR_AND_RECOVER("mpd_run_status");
    }
        
    if (!mpd_send_list_queue_range_meta(mpd_state->conn, offset, offset + mpd_state->max_elements_per_page)) {
        RETURN_ERROR_AND_RECOVER("mpd_send_list_queue_meta");
    }
        
    len = json_printf(&out, "{type: queue, data: [");

    while ((entity = mpd_recv_entity(mpd_state->conn)) != NULL && len < MAX_LIST_SIZE) {
        const struct mpd_song *song;
        if (mpd_entity_get_type(entity) == MPD_ENTITY_TYPE_SONG) {
            song = mpd_entity_get_song(entity);
            totalTime += mpd_song_get_duration(song);
            entity_count++;
            if (entities_returned++) 
                len += json_printf(&out, ",");
            len += json_printf(&out, "{id: %d, Pos: %d, ",
                mpd_song_get_id(song),
                mpd_song_get_pos(song)
            );
            PUT_SONG_TAG_COLS(tagcols);
            len += json_printf(&out, "}");
        }
        mpd_entity_free(entity);
    }

    len += json_printf(&out, "], totalTime: %d, totalEntities: %d, offset: %d, returnedEntities: %d, queue_version: %d}",
        totalTime,
        mpd_status_get_queue_length(status),
        offset,
        entities_returned,
        mpd_status_get_queue_version(status)
    );

    mpd_state->queue_version = mpd_status_get_queue_version(status);
    mpd_state->queue_length = mpd_status_get_queue_length(status);
    mpd_status_free(status);
    
    CHECK_RETURN_LEN();
}

static int mpd_client_put_browse(t_config *config, t_mpd_state *mpd_state, char *buffer, const char *path, const unsigned int offset, const char *filter, const t_tags *tagcols) {
    struct mpd_entity *entity;
    unsigned entity_count = 0;
    unsigned entities_returned = 0;
    const char *entityName;
    char smartpls_file[400];
    bool smartpls;
    size_t len = 0;
    struct json_out out = JSON_OUT_BUF(buffer, MAX_SIZE);

    if (!mpd_send_list_meta(mpd_state->conn, path)) {
        RETURN_ERROR_AND_RECOVER("mpd_send_list_meta");
    }

    len = json_printf(&out, "{type: browse, data: [");

    while ((entity = mpd_recv_entity(mpd_state->conn)) != NULL && len < MAX_LIST_SIZE) {
        entity_count++;
        if (entity_count > offset && entity_count <= offset + mpd_state->max_elements_per_page) {
            switch (mpd_entity_get_type(entity)) {
                case MPD_ENTITY_TYPE_UNKNOWN: {
                    entity_count--;
                    break;
                }
                case MPD_ENTITY_TYPE_SONG: {
                    const struct mpd_song *song = mpd_entity_get_song(entity);
                    entityName = mpd_client_get_tag(song, MPD_TAG_TITLE);
                    if (strncmp(filter, "-", 1) == 0 || strncasecmp(filter, entityName, 1) == 0 ||
                        (strncmp(filter, "0", 1) == 0 && isalpha(*entityName) == 0 )) 
                    {
                        if (entities_returned++) 
                            len += json_printf(&out, ",");
                        len += json_printf(&out, "{Type: song, ");
                        PUT_SONG_TAG_COLS(tagcols);
                        len += json_printf(&out, "}");
                    }
                    else {
                        entity_count--;
                    }
                    break;
                }
                case MPD_ENTITY_TYPE_DIRECTORY: {
                    const struct mpd_directory *dir = mpd_entity_get_directory(entity);                
                    entityName = mpd_directory_get_path(dir);
                    char *dirName = strrchr(entityName, '/');

                    if (dirName != NULL) {
                        dirName++;
                    }
                    else {
                        dirName = (char *) entityName;
                    }

                    if (strncmp(filter, "-", 1) == 0 || strncasecmp(filter, dirName, 1) == 0 ||
                        (strncmp(filter, "0", 1) == 0 && isalpha(*dirName) == 0 )) 
                    {                
                        if (entities_returned++) 
                            len += json_printf(&out, ",");
                        len += json_printf(&out, "{Type: dir, uri: %Q, name: %Q}",
                            entityName,
                            dirName
                        );
                    }
                    else {
                        entity_count--;
                    }
                    dirName = NULL;
                    break;
                }
                case MPD_ENTITY_TYPE_PLAYLIST: {
                    const struct mpd_playlist *pl = mpd_entity_get_playlist(entity);
                    entityName = mpd_playlist_get_path(pl);
                    char *plName = strrchr(entityName, '/');
                    if (plName != NULL) {
                        plName++;
                    } else {
                        plName = (char *) entityName;
                    }
                    if (strncmp(filter, "-", 1) == 0 || strncasecmp(filter, plName, 1) == 0 ||
                        (strncmp(filter, "0", 1) == 0 && isalpha(*plName) == 0 )) 
                    {
                        if (entities_returned++) {
                            len += json_printf(&out, ",");
                        }
                        if (validate_string(plName) == true) {
                            snprintf(smartpls_file, 400, "%s/smartpls/%s", config->varlibdir, plName);
                            if (access(smartpls_file, F_OK ) != -1) {
                                smartpls = true;
                            }
                            else {
                                smartpls = false;
                            }
                        }
                        else {
                            smartpls = false;
                        }                        
                        len += json_printf(&out, "{Type: %Q, uri: %Q, name: %Q}",
                            (smartpls == true ? "smartpls" : "plist"),
                            entityName,
                            plName
                        );
                    } else {
                        entity_count--;
                    }
                    plName = NULL;
                    break;
                }
            }
        }
        mpd_entity_free(entity);
    }

    mpd_response_finish(mpd_state->conn);

    len += json_printf(&out, "], totalEntities: %d, offset: %d, returnedEntities: %d, filter: %Q}",
        entity_count,
        offset,
        entities_returned,
        filter
    );

    CHECK_RETURN_LEN();
}

static int mpd_client_put_db_tag(t_mpd_state *mpd_state, char *buffer, const unsigned int offset, const char *mpdtagtype, const char *mpdsearchtagtype, const char *searchstr, const char *filter) {
    struct mpd_pair *pair;
    unsigned entity_count = 0;
    unsigned entities_returned = 0;
    size_t len = 0;
    struct json_out out = JSON_OUT_BUF(buffer, MAX_SIZE);

    if (mpd_search_db_tags(mpd_state->conn, mpd_tag_name_parse(mpdtagtype)) == false)
        RETURN_ERROR_AND_RECOVER("mpd_search_db_tags");

    if (mpd_tag_name_parse(mpdsearchtagtype) != MPD_TAG_UNKNOWN) {
        if (mpd_search_add_tag_constraint(mpd_state->conn, MPD_OPERATOR_DEFAULT, mpd_tag_name_parse(mpdsearchtagtype), searchstr) == false)
            RETURN_ERROR_AND_RECOVER("mpd_search_add_tag_constraint");
    }

    if (mpd_search_commit(mpd_state->conn) == false)
        RETURN_ERROR_AND_RECOVER("mpd_search_commit");

    len = json_printf(&out, "{type: listDBtags, data: [");
    while ((pair = mpd_recv_pair_tag(mpd_state->conn, mpd_tag_name_parse(mpdtagtype))) != NULL && len < MAX_LIST_SIZE) {
        entity_count++;
        if (entity_count > offset && entity_count <= offset + mpd_state->max_elements_per_page) {
            if (strcmp(pair->value, "") == 0) {
                entity_count--;
            }
            else if (strncmp(filter, "-", 1) == 0 || strncasecmp(filter, pair->value, 1) == 0 ||
                    (strncmp(filter, "0", 1) == 0 && isalpha(*pair->value) == 0 )) 
            {
                if (entities_returned++) 
                    len += json_printf(&out, ", ");
                len += json_printf(&out, "{type: %Q, value: %Q}",
                    mpdtagtype,
                    pair->value    
                );
            }
            else {
                entity_count--;
            }
        }
        mpd_return_pair(mpd_state->conn, pair);
    }
        
    len += json_printf(&out, "], totalEntities: %d, offset: %d, returnedEntities: %d, "
        "tagtype: %Q, searchtagtype: %Q, searchstr: %Q, filter: %Q}",
        entity_count,
        offset,
        entities_returned,
        mpdtagtype,
        mpdsearchtagtype,
        searchstr,
        filter
    );

    CHECK_RETURN_LEN();
}

static int mpd_client_put_songs_in_album(t_config *config, t_mpd_state *mpd_state, char *buffer, const char *album, const char *search, const char *tag, const t_tags *tagcols) {
    struct mpd_song *song;
    int entity_count = 0;
    int entities_returned = 0;
    size_t len = 0;
    size_t cover_len = 2000;
    char cover[cover_len];
    char *albumartist = NULL;
    int totalTime = 0;
    struct json_out out = JSON_OUT_BUF(buffer, MAX_SIZE);

    if (mpd_search_db_songs(mpd_state->conn, true) == false)
        RETURN_ERROR_AND_RECOVER("mpd_search_db_songs");
    
    if (mpd_search_add_tag_constraint(mpd_state->conn, MPD_OPERATOR_DEFAULT, mpd_tag_name_parse(tag), search) == false)
        RETURN_ERROR_AND_RECOVER("mpd_search_add_tag_constraint");

    if (mpd_search_add_tag_constraint(mpd_state->conn, MPD_OPERATOR_DEFAULT, MPD_TAG_ALBUM, album) == false)
        RETURN_ERROR_AND_RECOVER("mpd_search_add_tag_constraint");
        
    if (mpd_search_commit(mpd_state->conn) == false) {
        RETURN_ERROR_AND_RECOVER("mpd_search_commit");
    }
    else {
        len = json_printf(&out, "{type: listTitles, data: [");

        while ((song = mpd_recv_song(mpd_state->conn)) != NULL && len < MAX_LIST_SIZE) {
            entity_count++;
            if (entities_returned++) {
                len += json_printf(&out, ", ");
            }
            else {
                mpd_client_get_cover(config, mpd_state, mpd_song_get_uri(song), cover, cover_len);
                char *value = mpd_client_get_tag(song, MPD_TAG_ALBUM_ARTIST);
                if (value != NULL) {
                    albumartist = strdup(value);
                }
            }
            len += json_printf(&out, "{Type: song, ");
            PUT_SONG_TAG_COLS(tagcols);
            len += json_printf(&out, "}");
            totalTime += mpd_song_get_duration(song);
            mpd_song_free(song);
        }
        
        len += json_printf(&out, "], totalEntities: %d, returnedEntities: %d, Album: %Q, search: %Q, tag: %Q, cover: %Q, AlbumArtist: %Q, totalTime: %d}",
            entity_count,
            entities_returned,
            album,
            search,
            tag,
            cover,
            (albumartist != NULL ? albumartist : "-"),
            totalTime
        );
    }
    FREE_PTR(albumartist);

    CHECK_RETURN_LEN();
}

static int mpd_client_rename_playlist(t_config *config, t_mpd_state *mpd_state, char *buffer, const char *old_playlist, const char *new_playlist) {
    struct json_out out = JSON_OUT_BUF(buffer, MAX_SIZE);
    size_t old_pl_file_len = config->varlibdir_len + strlen(old_playlist) + 11;
    char old_pl_file[old_pl_file_len];
    size_t new_pl_file_len = config->varlibdir_len + strlen(new_playlist) + 11;
    char new_pl_file[new_pl_file_len];
    size_t len = 0;
    
    if (validate_string(old_playlist) == false || validate_string(new_playlist) == false) {
        len = json_printf(&out, "{type: error, data: %Q}", "Invalid filename");
        return len;
    }
    
    snprintf(old_pl_file, old_pl_file_len, "%s/smartpls/%s", config->varlibdir, old_playlist);
    snprintf(new_pl_file, new_pl_file_len, "%s/smartpls/%s", config->varlibdir, new_playlist);
    
    if (access(old_pl_file, F_OK ) != -1) {
        //smart playlist
        if (access(new_pl_file, F_OK ) == -1) {
            //new playlist doesn't exist
            if (rename(old_pl_file, new_pl_file) == -1) {
                len = json_printf(&out, "{type: error, data: %Q}", "Renaming playlist failed");
                LOG_ERROR("Renaming smart playlist %s to %s failed", old_pl_file, new_pl_file);
                return len;
            }
        } 
    }
    
    if (mpd_run_rename(mpd_state->conn, old_playlist, new_playlist)) {
        len = json_printf(&out, "{type: result, data: %Q}", "Sucessfully renamed playlist");
    }
    else {
        RETURN_ERROR_AND_RECOVER("mpd_run_rename");
    }

    CHECK_RETURN_LEN();
}

static int mpd_client_put_playlists(t_config *config, t_mpd_state *mpd_state, char *buffer, const unsigned int offset, const char *filter) {
    struct mpd_playlist *pl;
    unsigned entity_count = 0;
    unsigned entities_returned = 0;
    const char *plpath;
    size_t len = 0;
    bool smartpls;
    struct json_out out = JSON_OUT_BUF(buffer, MAX_SIZE);

    if (mpd_send_list_playlists(mpd_state->conn) == false) {
        RETURN_ERROR_AND_RECOVER("mpd_send_lists_playlists");
    }

    len = json_printf(&out, "{type: playlists, data: [");

    while ((pl = mpd_recv_playlist(mpd_state->conn)) != NULL && len < MAX_LIST_SIZE) {
        entity_count++;
        if (entity_count > offset && entity_count <= offset + mpd_state->max_elements_per_page) {
            plpath = mpd_playlist_get_path(pl);
            if (strncmp(filter, "-", 1) == 0 || strncasecmp(filter, plpath, 1) == 0 ||
               (strncmp(filter, "0", 1) == 0 && isalpha(*plpath) == 0 )) 
            {
                if (entities_returned++)
                    len += json_printf(&out, ", ");
                size_t smartpls_file_len = config->varlibdir_len + strlen(plpath) + 11;
                char smartpls_file[smartpls_file_len];
                snprintf(smartpls_file, smartpls_file_len, "%s/smartpls/%s", config->varlibdir, plpath);
                if (validate_string(plpath) == true) {
                    if (access(smartpls_file, F_OK ) != -1) {
                        smartpls = true;
                    }
                    else {
                        smartpls = false;
                    }
                }
                else {
                    smartpls = false;
                }
                len += json_printf(&out, "{Type: %Q, uri: %Q, name: %Q, last_modified: %d}",
                    (smartpls == true ? "smartpls" : "plist"), 
                    plpath,
                    plpath,
                    mpd_playlist_get_last_modified(pl)
                );
            } else {
                entity_count--;
            }
        }
        mpd_playlist_free(pl);
    }

    len += json_printf(&out, "], totalEntities: %d, offset: %d, returnedEntities: %d}",
        entity_count,
        offset,
        entities_returned
    );

    CHECK_RETURN_LEN();
}

static int mpd_client_put_playlist_list(t_config *config, t_mpd_state *mpd_state, char *buffer, const char *uri, const unsigned int offset, const char *filter, const t_tags *tagcols) {
    struct mpd_entity *entity;
    unsigned entities_returned = 0;
    unsigned entity_count = 0;
    const char *entityName;
    bool smartpls = false;
    size_t len = 0;
    struct json_out out = JSON_OUT_BUF(buffer, MAX_SIZE);

    if (mpd_send_list_playlist_meta(mpd_state->conn, uri) == false) {
        RETURN_ERROR_AND_RECOVER("mpd_send_list_meta");
    }

    len = json_printf(&out, "{type: playlist_detail, data: [");

    while ((entity = mpd_recv_entity(mpd_state->conn)) != NULL && len < MAX_LIST_SIZE) {
        const struct mpd_song *song;
        entity_count++;
        if (entity_count > offset && entity_count <= offset + mpd_state->max_elements_per_page) {
            song = mpd_entity_get_song(entity);
            entityName = mpd_client_get_tag(song, MPD_TAG_TITLE);
            if (strncmp(filter, "-", 1) == 0 || strncasecmp(filter, entityName, 1) == 0 ||
               (strncmp(filter, "0", 1) == 0 && isalpha(*entityName) == 0))
            {
                if (entities_returned++) {
                    len += json_printf(&out, ",");
                }
                len += json_printf(&out, "{Type: song, ");
                PUT_SONG_TAG_COLS(tagcols);
                len += json_printf(&out, ", Pos: %d", entity_count);
                len += json_printf(&out, "}");
            }
            else {
                entity_count--;
            }
        }
        mpd_entity_free(entity);
    }
    mpd_response_finish(mpd_state->conn);
    
    if (validate_string(uri) == true) {
        size_t smartpls_file_len = config->varlibdir_len + strlen(uri) + 11;
        char smartpls_file[smartpls_file_len];
        snprintf(smartpls_file, smartpls_file_len, "%s/smartpls/%s", config->varlibdir, uri);
        if (access(smartpls_file, F_OK ) != -1) {
            smartpls = true;
        }
    }
    len += json_printf(&out, "], totalEntities: %d, offset: %d, returnedEntities: %d, filter: %Q, uri: %Q, smartpls: %B}",
        entity_count,
        offset,
        entities_returned,
        filter,
        uri,
        smartpls
    );

    CHECK_RETURN_LEN();
}

static int mpd_client_search(t_mpd_state *mpd_state, char *buffer, const char *searchstr, const char *filter, const char *plist, const unsigned int offset, const t_tags *tagcols) {
    struct mpd_song *song;
    unsigned entity_count = 0;
    unsigned entities_returned = 0;
    size_t len = 0;
    struct json_out out = JSON_OUT_BUF(buffer, MAX_SIZE);
    
    if (strcmp(plist, "") == 0) {
        if (mpd_send_command(mpd_state->conn, "search", filter, searchstr, NULL) == false)
            RETURN_ERROR_AND_RECOVER("mpd_search");
        len = json_printf(&out, "{type: search, data: [");
    }
    else if (strcmp(plist, "queue") == 0) {
        if (mpd_send_command(mpd_state->conn, "searchadd", filter, searchstr, NULL) == false)
            RETURN_ERROR_AND_RECOVER("mpd_searchadd");
    }
    else {
        if (mpd_send_command(mpd_state->conn, "searchaddpl", plist, filter, searchstr, NULL) == false)
            RETURN_ERROR_AND_RECOVER("mpd_searchaddpl");
    }

    if (strcmp(plist, "") == 0) {
        while ((song = mpd_recv_song(mpd_state->conn)) != NULL && len < MAX_LIST_SIZE) {
            entity_count++;
            if (entity_count > offset && entity_count <= offset + mpd_state->max_elements_per_page) {
                if (entities_returned++) 
                    len += json_printf(&out, ", ");
                len += json_printf(&out, "{Type: song, ");
                PUT_SONG_TAG_COLS(tagcols);
                len += json_printf(&out, "}");
            }
            mpd_song_free(song);
        }
    }
    else {
        mpd_response_finish(mpd_state->conn);
    }

    if (strcmp(plist, "") == 0) {
        len += json_printf(&out, "], totalEntities: %d, offset: %d, returnedEntities: %d, searchstr: %Q}",
            entity_count,
            offset,
            entities_returned,
            searchstr
        );
    } 
    else {
        len = json_printf(&out, "{type: result, data: ok}");
    }

    CHECK_RETURN_LEN();
}


static int mpd_client_search_adv(t_mpd_state *mpd_state, char *buffer, const char *expression, const char *sort, const bool sortdesc, const char *grouptag, const char *plist, const unsigned int offset, const t_tags *tagcols) {
    size_t len = 0;
    struct json_out out = JSON_OUT_BUF(buffer, MAX_SIZE);    
#if LIBMPDCLIENT_CHECK_VERSION(2, 17, 0)
    struct mpd_song *song;
    unsigned entities_returned = 0;
    
    if (strcmp(plist, "") == 0) {
        if (mpd_search_db_songs(mpd_state->conn, false) == false)
            RETURN_ERROR_AND_RECOVER("mpd_search_db_songs");
        len = json_printf(&out, "{type: search, data: [");
    }
    else if (strcmp(plist, "queue") == 0) {
        if (mpd_search_add_db_songs(mpd_state->conn, false) == false)
            RETURN_ERROR_AND_RECOVER("mpd_search_add_db_songs");
    }
    else {
        if (mpd_search_add_db_songs_to_playlist(mpd_state->conn, plist) == false)
            RETURN_ERROR_AND_RECOVER("mpd_search_add_db_songs_to_playlist");
    }
    
    if (mpd_search_add_expression(mpd_state->conn, expression) == false)
        RETURN_ERROR_AND_RECOVER("mpd_search_add_expression");

    if (strcmp(plist, "") == 0) {
        if (sort != NULL && strcmp(sort, "") != 0 && strcmp(sort, "-") != 0 && mpd_state->feat_tags == true) {
            if (mpd_search_add_sort_name(mpd_state->conn, sort, sortdesc) == false)
                RETURN_ERROR_AND_RECOVER("mpd_search_add_sort_name");
        }
        if (grouptag != NULL && strcmp(grouptag, "") != 0 && mpd_state->feat_tags == true) {
            if (mpd_search_add_group_tag(mpd_state->conn, mpd_tag_name_parse(grouptag)) == false)
                RETURN_ERROR_AND_RECOVER("mpd_search_add_group_tag");
        }
        if (mpd_search_add_window(mpd_state->conn, offset, offset + mpd_state->max_elements_per_page) == false)
            RETURN_ERROR_AND_RECOVER("mpd_search_add_window");
    }
    
    if (mpd_search_commit(mpd_state->conn) == false)
        RETURN_ERROR_AND_RECOVER("mpd_search_commit");

    if (strcmp(plist, "") == 0) {
        while ((song = mpd_recv_song(mpd_state->conn)) != NULL && len < MAX_LIST_SIZE) {
            if (entities_returned++) 
                len += json_printf(&out, ", ");
            len += json_printf(&out, "{Type: song, ");
            PUT_SONG_TAG_COLS(tagcols);
            len += json_printf(&out, "}");
            mpd_song_free(song);
        }
    }
    else {
        mpd_response_finish(mpd_state->conn);
    }

    if (strcmp(plist, "") == 0) {
        len += json_printf(&out, "], totalEntities: %d, offset: %d, returnedEntities: %u, expression: %Q, "
            "sort: %Q, sortdesc: %B, grouptag: %Q}",
            -1,
            offset,
            entities_returned,
            expression,
            sort,
            sortdesc,
            grouptag
        );
    } 
    else {
        len = json_printf(&out, "{type: result, data: ok}");
    }
#else
    //prevent unused warnings
    (void)(mpd_state);
    (void)(expression);
    (void)(sort);
    (void)(sortdesc);
    (void)(grouptag);
    (void)(plist);
    (void)(offset);
    (void)(tagcols);
    len = json_printf(&out, "{type: error, data: %Q}", "Advanced search is disabled");
#endif
    CHECK_RETURN_LEN();
}

static int mpd_client_queue_crop(t_mpd_state *mpd_state, char *buffer) {
    size_t len = 0;
    struct json_out out = JSON_OUT_BUF(buffer, MAX_SIZE);
    struct mpd_status *status = mpd_run_status(mpd_state->conn);
    const int length = mpd_status_get_queue_length(status) - 1;
    int playing_song_pos = mpd_status_get_song_pos(status);

    if (length < 1) {
        len = json_printf(&out, "{type: error, data: %Q}", "A queue longer than 1 song in length is required to crop");
        LOG_ERROR("A playlist longer than 1 song in length is required to crop");
    }
    else if (mpd_status_get_state(status) == MPD_STATE_PLAY || mpd_status_get_state(status) == MPD_STATE_PAUSE) {
        playing_song_pos++;
        if (playing_song_pos < length) {
            mpd_run_delete_range(mpd_state->conn, playing_song_pos, -1);
        }
        playing_song_pos--;
        if (playing_song_pos > 0) {
            mpd_run_delete_range(mpd_state->conn, 0, playing_song_pos--);
        }
        len = json_printf(&out, "{type: result, data: ok}");
    } else {
        len = json_printf(&out, "{type: error, data: %Q}", "You need to be playing to crop the playlist");
        LOG_ERROR("You need to be playing to crop the playlist");
    }
    
    mpd_status_free(status);
    
    CHECK_RETURN_LEN();
}

static int mpd_client_search_queue(t_mpd_state *mpd_state, char *buffer, const char *mpdtagtype, const unsigned int offset, const char *searchstr, const t_tags *tagcols) {
    struct mpd_song *song;
    unsigned entity_count = 0;
    unsigned entities_returned = 0;
    size_t len = 0;
    struct json_out out = JSON_OUT_BUF(buffer, MAX_SIZE);

    if (mpd_search_queue_songs(mpd_state->conn, false) == false) {
        RETURN_ERROR_AND_RECOVER("mpd_search_queue_songs");
    }
    
    if (mpd_tag_name_parse(mpdtagtype) != MPD_TAG_UNKNOWN) {
       if (mpd_search_add_tag_constraint(mpd_state->conn, MPD_OPERATOR_DEFAULT, mpd_tag_name_parse(mpdtagtype), searchstr) == false)
            RETURN_ERROR_AND_RECOVER("mpd_search_add_tag_constraint");
    }
    else {
        if (mpd_search_add_any_tag_constraint(mpd_state->conn, MPD_OPERATOR_DEFAULT, searchstr) == false)
            RETURN_ERROR_AND_RECOVER("mpd_search_add_any_tag_constraint");        
    }

    if (mpd_search_commit(mpd_state->conn) == false) {
        RETURN_ERROR_AND_RECOVER("mpd_search_commit");
    }
    else {
        len = json_printf(&out, "{type: queuesearch, data: [");

        while ((song = mpd_recv_song(mpd_state->conn)) != NULL && len < MAX_LIST_SIZE) {
            entity_count++;
            if (entity_count > offset && entity_count <= offset + mpd_state->max_elements_per_page) {
                if (entities_returned++)
                    len += json_printf(&out, ", ");
                len += json_printf(&out, "{type: song, id: %d, Pos: %d, ",
                    mpd_song_get_id(song),
                    mpd_song_get_pos(song)
                );
                PUT_SONG_TAG_COLS(tagcols);
                len += json_printf(&out, "}");
                mpd_song_free(song);
            }
        }
        
        len += json_printf(&out, "], totalEntities: %d, offset: %d, returnedEntities: %d, mpdtagtype: %Q}",
            entity_count,
            offset,
            entities_returned,
            mpdtagtype
        );
    }

    CHECK_RETURN_LEN();
}

static int mpd_client_put_stats(t_mpd_state *mpd_state, char *buffer) {
    struct mpd_stats *stats = mpd_run_stats(mpd_state->conn);
    const unsigned *version = mpd_connection_get_server_version(mpd_state->conn);
    size_t len = 0;
    char mpd_version[20];
    char libmpdclient_version[20];
    struct json_out out = JSON_OUT_BUF(buffer, MAX_SIZE);
    snprintf(mpd_version, 20, "%u.%u.%u", version[0], version[1], version[2]);
    snprintf(libmpdclient_version, 20, "%i.%i.%i", LIBMPDCLIENT_MAJOR_VERSION, LIBMPDCLIENT_MINOR_VERSION, LIBMPDCLIENT_PATCH_VERSION);

    if (stats == NULL) {
        RETURN_ERROR_AND_RECOVER("mpd_run_stats");
    }
    len = json_printf(&out, "{type: mpdstats, data: {artists: %d, albums: %d, songs: %d, "
        "playtime: %d, uptime: %d, dbUpdated: %d, dbPlaytime: %d, mympdVersion: %Q, mpdVersion: %Q, "
        "libmpdclientVersion: %Q}}",
        mpd_stats_get_number_of_artists(stats),
        mpd_stats_get_number_of_albums(stats),
        mpd_stats_get_number_of_songs(stats),
        mpd_stats_get_play_time(stats),
        mpd_stats_get_uptime(stats),
        mpd_stats_get_db_update_time(stats),
        mpd_stats_get_db_play_time(stats),
        MYMPD_VERSION,
        mpd_version,
        libmpdclient_version
    );
    mpd_stats_free(stats);

    CHECK_RETURN_LEN();
}

static void mpd_client_disconnect(t_mpd_state *mpd_state) {
    mpd_state->conn_state = MPD_DISCONNECT;
    if (mpd_state->conn != NULL) {
        mpd_connection_free(mpd_state->conn);
    }
}

static int mpd_client_smartpls_put(t_config *config, char *buffer, const char *playlist) {
    size_t pl_file_len = config->varlibdir_len + 11 + strlen(playlist);
    char pl_file[pl_file_len];
    char *smartpltype = NULL;
    char *p_charbuf1 = NULL;
    char *p_charbuf2 = NULL;
    int je, int_buf1;
    size_t len = 0;
    struct json_out out = JSON_OUT_BUF(buffer, MAX_SIZE);
    
    if (validate_string(playlist) == false) {
        len = json_printf(&out, "{type: error, data: %Q}}", "Can not read smart playlist file");
        return len;
    }
    snprintf(pl_file, pl_file_len, "%s/smartpls/%s", config->varlibdir, playlist);
    char *content = json_fread(pl_file);
    if (content == NULL) {
        len = json_printf(&out, "{type: error, data: %Q}}", "Can not read smart playlist file");
        LOG_ERROR("Can't read smart playlist file: %s", pl_file);
        return len;
    }
    je = json_scanf(content, strlen(content), "{type: %Q }", &smartpltype);
    if (je == 1) {
        if (strcmp(smartpltype, "sticker") == 0) {
            je = json_scanf(content, strlen(content), "{sticker: %Q, maxentries: %d}", &p_charbuf1, &int_buf1);
            if (je == 2) {
                len = json_printf(&out, "{type: smartpls, data: {playlist: %Q, type: %Q, sticker: %Q, maxentries: %d}}",
                    playlist,
                    smartpltype,
                    p_charbuf1,
                    int_buf1);
                FREE_PTR(p_charbuf1);
            } else {
                len = json_printf(&out, "{type: error, data: %Q]", "Can not parse smart playlist file");
                LOG_ERROR("Can't parse smart playlist file: %s", pl_file);
            }
        }
        else if (strcmp(smartpltype, "newest") == 0) {
            je = json_scanf(content, strlen(content), "{timerange: %d}", &int_buf1);
            if (je == 1) {
                len = json_printf(&out, "{type: smartpls, data: {playlist: %Q, type: %Q, timerange: %d}}",
                    playlist,
                    smartpltype,
                    int_buf1);
            } else {
                len = json_printf(&out, "{type: error, data: %Q]", "Can not parse smart playlist file");
                LOG_ERROR("Can't parse smart playlist file: %s", pl_file);
            }
        }
        else if (strcmp(smartpltype, "search") == 0) {
            je = json_scanf(content, strlen(content), "{tag: %Q, searchstr: %Q}", &p_charbuf1, &p_charbuf2);
            if (je == 2) {
                len = json_printf(&out, "{type: smartpls, data: {playlist: %Q, type: %Q, tag: %Q, searchstr: %Q}}",
                    playlist,
                    smartpltype,
                    p_charbuf1,
                    p_charbuf2);
                FREE_PTR(p_charbuf1);
                FREE_PTR(p_charbuf2);
            } else {
                len = json_printf(&out, "{type: error, data: %Q]", "Can not parse smart playlist file");
                LOG_ERROR("Can't parse smart playlist file: %s", pl_file);
            }
        } else {
            len = json_printf(&out, "{type: error, data: %Q}}", "Unknown smart playlist type");
            LOG_ERROR("Unknown smart playlist type: %s", pl_file);
        }
        FREE_PTR(smartpltype);        
    } else {
        len = json_printf(&out, "{type: error, data: %Q}}", "Unknown smart playlist type");
        LOG_ERROR("Unknown smart playlist type: %s", pl_file);
    }
    FREE_PTR(content);
    return len;
}

static bool mpd_client_smartpls_save(t_config *config, t_mpd_state *mpd_state, const char *smartpltype, const char *playlist, const char *tag, const char *searchstr, const int maxentries, const int timerange) {
    size_t tmp_file_len = config->varlibdir_len + strlen(playlist) + 18;
    char tmp_file[tmp_file_len];
    size_t pl_file_len = config->varlibdir_len + strlen(playlist) + 11;
    char pl_file[pl_file_len];
    int fd;
    
    if (validate_string(playlist) == false) {
        return false;
    }
    
    snprintf(tmp_file, tmp_file_len, "%s/smartpls/%s.XXXXXX", config->varlibdir, playlist);
    snprintf(pl_file, pl_file_len, "%s/smartpls/%s", config->varlibdir, playlist);
    
    if ((fd = mkstemp(tmp_file)) < 0 ) {
        LOG_ERROR("Can't open %s for write", tmp_file);
        return false;
    }
    FILE *fp = fdopen(fd, "w");
    struct json_out out = JSON_OUT_FILE(fp);
    
    if (strcmp(smartpltype, "sticker") == 0) {
        if (json_printf(&out, "{type: %Q, sticker: %Q, maxentries: %d}", smartpltype, tag, maxentries) == -1) {
            LOG_ERROR("Can't write to file %s", tmp_file);
            fclose(fp);
            return false;
        }
        fclose(fp);
        if (rename(tmp_file, pl_file) == -1) {
            LOG_ERROR("Renaming file from %s to %s failed", tmp_file, pl_file);
            return false;
        }
        else if (mpd_client_smartpls_update_sticker(mpd_state, playlist, tag, maxentries) == false) {
            LOG_ERROR("Update of smart playlist %s failed.", playlist);
            return false;
        }
    }
    else if (strcmp(smartpltype, "newest") == 0) {
        if (json_printf(&out, "{type: %Q, timerange: %d}", smartpltype, timerange) == -1) {
            LOG_ERROR("Can't write to file %s", tmp_file);
            fclose(fp);
            return false;
        }
        fclose(fp);
        if (rename(tmp_file, pl_file) == -1) {
            LOG_ERROR("Renaming file from %s to %s failed", tmp_file, pl_file);
            return false;
        }
        else if (mpd_client_smartpls_update_newest(mpd_state, playlist, timerange) == false) {
            LOG_ERROR("Update of smart playlist %s failed", playlist);
            return false;
        }
    }
    else if (strcmp(smartpltype, "search") == 0) {
        if (json_printf(&out, "{type: %Q, tag: %Q, searchstr: %Q}", smartpltype, tag, searchstr) == -1) {
            LOG_ERROR("Can't write to file %s", tmp_file);
            fclose(fp);
            return false;
        }
        fclose(fp);
        if (rename(tmp_file, pl_file) == -1) {
            LOG_ERROR("Renaming file from %s to %s failed", tmp_file, pl_file);
            return false;
        }
        else if (mpd_client_smartpls_update_search(mpd_state, playlist, tag, searchstr) == false) {
            LOG_ERROR("Update of smart playlist %s failed", playlist);
            return false;
        }
    }
    return true;
}

static bool mpd_client_smartpls_update_all(t_config *config, t_mpd_state *mpd_state) {
    DIR *dir;
    struct dirent *ent;

    if (mpd_state->feat_smartpls == false) {
        return true;
    }
    
    size_t dirname_len = config->varlibdir_len + 10;
    char dirname[dirname_len];
    snprintf(dirname, dirname_len, "%s/smartpls", config->varlibdir);
    if ((dir = opendir (dirname)) != NULL) {
        while ((ent = readdir(dir)) != NULL) {
            if (strncmp(ent->d_name, ".", 1) == 0) {
                continue;
            }
            else {
                mpd_client_smartpls_update(config, mpd_state, ent->d_name);
            }
        }
        closedir (dir);
    } else {
        LOG_ERROR("Can't open smart playlist directory %s", dirname);
        return false;
    }
    return true;
}

static bool mpd_client_smartpls_update(t_config *config, t_mpd_state *mpd_state, char *playlist) {
    char *smartpltype = NULL;
    int je;
    bool rc = true;
    char *p_charbuf1 = NULL;
    char *p_charbuf2 = NULL;
    int int_buf1;
    if (mpd_state->feat_smartpls == false) {
        return true;
    }
    
    size_t filename_len = config->varlibdir_len + strlen(playlist) + 11;
    char filename[filename_len];
    snprintf(filename, filename_len, "%s/smartpls/%s", config->varlibdir, playlist);
    char *content = json_fread(filename);
    if (content == NULL) {
        LOG_ERROR("Cant read smart playlist file %s", filename);
        return false;
    }
    je = json_scanf(content, strlen(content), "{type: %Q }", &smartpltype);
    if (je != 1) {
        LOG_ERROR("Cant read smart playlist type from %s", filename);
        return false;
    }
    if (strcmp(smartpltype, "sticker") == 0) {
        je = json_scanf(content, strlen(content), "{sticker: %Q, maxentries: %d}", &p_charbuf1, &int_buf1);
        if (je == 2) {
            if (mpd_client_smartpls_update_sticker(mpd_state, playlist, p_charbuf1, int_buf1) == false) {
                LOG_ERROR("Update of smart playlist %s failed.", playlist);
                rc = false;
            }
            FREE_PTR(p_charbuf1);
         }
         else {
            LOG_ERROR("Can't parse smart playlist file %s", filename);
            rc = false;
         }
    }
    else if (strcmp(smartpltype, "newest") == 0) {
        je = json_scanf(content, strlen(content), "{timerange: %d}", &int_buf1);
        if (je == 1) {
            if (mpd_client_smartpls_update_newest(mpd_state, playlist, int_buf1) == false) {
                LOG_ERROR("Update of smart playlist %s failed", playlist);
                rc = false;
            }
        }
        else {
            LOG_ERROR("Can't parse smart playlist file %s", filename);
            rc = false;
        }
    }
    else if (strcmp(smartpltype, "search") == 0) {
        je = json_scanf(content, strlen(content), "{tag: %Q, searchstr: %Q}", &p_charbuf1, &p_charbuf2);
        if (je == 2) {
            if (mpd_client_smartpls_update_search(mpd_state, playlist, p_charbuf1, p_charbuf2) == false) {
                LOG_ERROR("Update of smart playlist %s failed", playlist);
                rc = false;
            }
            FREE_PTR(p_charbuf1);
            FREE_PTR(p_charbuf2);
        }
        else {
            LOG_ERROR("Can't parse smart playlist file %s", filename);
            rc = false;
        }
    }
    FREE_PTR(smartpltype);
    FREE_PTR(content);
    return rc;
}

static bool mpd_client_smartpls_clear(t_mpd_state *mpd_state, const char *playlist) {
    struct mpd_playlist *pl;
    const char *plpath;
    bool exists = false;
    if (mpd_send_list_playlists(mpd_state->conn) == false) {
        LOG_ERROR_AND_RECOVER("mpd_send_list_playlists");
        return 1;
    }
    while ((pl = mpd_recv_playlist(mpd_state->conn)) != NULL) {
        plpath = mpd_playlist_get_path(pl);
        if (strcmp(playlist, plpath) == 0)
            exists = true;
        mpd_playlist_free(pl);
        if (exists == true) {
            break;
        }
    }
    mpd_response_finish(mpd_state->conn);
    
    if (exists) {
        if (mpd_run_rm(mpd_state->conn, playlist) == false) {
            LOG_ERROR_AND_RECOVER("mpd_run_rm");
            return false;
        }
    }
    return true;
}

static bool mpd_client_smartpls_update_search(t_mpd_state *mpd_state, const char *playlist, const char *tag, const char *searchstr) {
    char buffer[MAX_SIZE];
    mpd_client_smartpls_clear(mpd_state, playlist);
    if (mpd_state->feat_advsearch == true && strcmp(tag, "expression") == 0) {
        mpd_client_search_adv(mpd_state, buffer, searchstr, NULL, true, NULL, playlist, 0, NULL);
    }
    else {
        mpd_client_search(mpd_state, buffer, searchstr, tag, playlist, 0, NULL);
    }
    LOG_INFO("Updated smart playlist %s", playlist);
    return true;
}

static bool mpd_client_smartpls_update_sticker(t_mpd_state *mpd_state, const char *playlist, const char *sticker, const int maxentries) {
    struct mpd_pair *pair;
    char *uri = NULL;
    const char *p_value;
    char *crap = NULL;
    int value;
    int value_max = 0;
    int i = 0;
    size_t j;

    if (mpd_send_sticker_find(mpd_state->conn, "song", "", sticker) == false) {
        LOG_ERROR_AND_RECOVER("mpd_send_sticker_find");
        return false;    
    }

    struct list add_list;
    list_init(&add_list);

    while ((pair = mpd_recv_pair(mpd_state->conn)) != NULL) {
        if (strcmp(pair->name, "file") == 0) {
            FREE_PTR(uri);
            uri = strdup(pair->value);
        } 
        else if (strcmp(pair->name, "sticker") == 0) {
            p_value = mpd_parse_sticker(pair->value, &j);
            if (p_value != NULL) {
                value = strtoimax(p_value, &crap, 10);
                if (value >= 1) {
                    list_push(&add_list, uri, value);
                }
                if (value > value_max) {
                    value_max = value;
                }
            }
        }
        mpd_return_pair(mpd_state->conn, pair);
    }
    mpd_response_finish(mpd_state->conn);
    FREE_PTR(uri);

    mpd_client_smartpls_clear(mpd_state, playlist);
     
    if (value_max > 2) {
        value_max = value_max / 2;
    }

    list_sort_by_value(&add_list, false);

    struct node *current = add_list.list;
    while (current != NULL) {
        if (current->value >= value_max) {
            if (mpd_run_playlist_add(mpd_state->conn, playlist, current->data) == false) {
                LOG_ERROR_AND_RECOVER("mpd_run_playlist_add");
                list_free(&add_list);
                return 1;        
            }
            i++;
            if (i >= maxentries)
                break;
        }
        current = current->next;
    }
    list_free(&add_list);
    LOG_INFO("Updated smart playlist %s with %d songs, minValue: %d", playlist, i, value_max);
    return true;
}

static bool mpd_client_smartpls_update_newest(t_mpd_state *mpd_state, const char *playlist, const int timerange) {
    int value_max = 0;
    char buffer[MAX_SIZE];
    char searchstr[50];
    
    struct mpd_stats *stats = mpd_run_stats(mpd_state->conn);
    if (stats != NULL) {
        value_max = mpd_stats_get_db_update_time(stats);
        mpd_stats_free(stats);
    }
    else {
        LOG_ERROR_AND_RECOVER("mpd_run_stats");
        return false;
    }

    mpd_client_smartpls_clear(mpd_state, playlist);
    value_max -= timerange;
    if (value_max > 0) {
        if (mpd_state->feat_advsearch == true) {
            snprintf(searchstr, 50, "(modified-since '%d')", value_max);
            mpd_client_search_adv(mpd_state, buffer, searchstr, NULL, true, NULL, playlist, 0, NULL);
        }
        else {
            snprintf(searchstr, 20, "%d", value_max);
            mpd_client_search(mpd_state, buffer, searchstr, "modified-since", playlist, 0, NULL);
        }
        LOG_INFO("Updated smart playlist %s", playlist);
    }
    return true;
}
