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

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <libgen.h>
#include <ctype.h>
#include <libgen.h>
#include <poll.h>
#include <dirent.h>
#include <mpd/client.h>

#include "list.h"
#include "tiny_queue.h"
#include "global.h"
#include "mpd_client.h"
#include "../dist/src/frozen/frozen.h"

//private definitions
#define RETURN_ERROR_AND_RECOVER(X) do { \
    printf("MPD %s: %s\n", X, mpd_connection_get_error_message(mpd_state->conn)); \
    len = json_printf(&out, "{type: error, data: %Q}", mpd_connection_get_error_message(mpd_state->conn)); \
    if (!mpd_connection_clear_error(mpd_state->conn)) \
        mpd_state->conn_state = MPD_FAILURE; \
    return len; \
} while (0)

#define LOG_ERROR_AND_RECOVER(X) do { \
    printf("MPD %s: %s\n", X, mpd_connection_get_error_message(mpd_state->conn)); \
    if (!mpd_connection_clear_error(mpd_state->conn)) \
        mpd_state->conn_state = MPD_FAILURE; \
} while (0)

#define PUT_SONG_TAGS() do { \
    struct node *current_tag = mpd_state->mympd_tags.list; \
    int tagnr = 0; \
    while (current_tag != NULL) { \
        if (tagnr ++) \
            len += json_printf(&out, ","); \
        char *value = mpd_client_get_tag(song, mpd_tag_name_parse(current_tag->data)); \
        len += json_printf(&out, "%Q: %Q", current_tag->data, value); \
        if (strncmp(value, "-", 1) == 0) { free(value); } \
        current_tag = current_tag->next; \
    } \
    len += json_printf(&out, ", Duration: %d, uri: %Q", mpd_song_get_duration(song), mpd_song_get_uri(song)); \
} while (0)

#define PUT_MIN_SONG_TAGS() do { \
    char *value = mpd_client_get_tag(song, MPD_TAG_TITLE); \
    len += json_printf(&out, "Title: %Q, Duration: %d, uri: %Q", value, mpd_song_get_duration(song), mpd_song_get_uri(song)); \
    if (strncmp(value, "-", 1) == 0) { free(value); } \
} while (0)


enum mpd_conn_states {
    MPD_DISCONNECTED,
    MPD_FAILURE,
    MPD_CONNECTED,
    MPD_RECONNECT,
    MPD_DISCONNECT
};

typedef struct t_mpd_state {
    // Connection
    struct mpd_connection *conn;
    enum mpd_conn_states conn_state;
    int timeout;

    // States
    int song_id;
    int next_song_id;
    int last_song_id;
    unsigned queue_version;
    unsigned queue_length;
    int last_update_sticker_song_id;
    int last_last_played_id;
    
    // Features
    const unsigned* protocol;
    bool feat_sticker;
    bool feat_playlists;
    bool feat_tags;
    bool feat_library;
    bool feat_advsearch;
    bool feat_smartpls;
    
    //mympd states
    enum jukebox_modes jukeboxMode;
    char *jukeboxPlaylist;
    int jukeboxQueueLength;
    bool autoPlay;
    
    //taglists
    struct list mpd_tags;
    struct list mympd_tags;
    struct list mympd_searchtags;
    struct list mympd_browsetags;
    
    //last played list
    struct list last_played;
} t_mpd_state;

typedef struct t_sticker {
    long playCount;
    long skipCount;
    long lastPlayed;
    long like;
} t_sticker;

static void mpd_client_idle(t_config *config, t_mpd_state *mpd_state);
static void mpd_client_parse_idle(t_config *config, t_mpd_state *mpd_state, const int idle_bitmask);
static void mpd_client_api(t_config *config, t_mpd_state *mpd_state, void *arg_request);
static void mpd_client_notify(const char *message, const size_t n);
static bool mpd_client_count_song_id(t_mpd_state *mpd_state, const int song_id, const char *name, const int value);
static bool mpd_client_count_song_uri(t_mpd_state *mpd_state, const char *uri, const char *name, const int value);
static bool mpd_client_like_song_uri(t_mpd_state *mpd_state, const char *uri, int value);
static bool mpd_client_last_played_song_id(t_mpd_state *mpd_state, const int song_id);
static bool mpd_client_last_played_song_uri(t_mpd_state *mpd_state, const char *uri);
static bool mpd_client_get_sticker(t_mpd_state *mpd_state, const char *uri, t_sticker *sticker);
static bool mpd_client_last_played_list(t_config *config, t_mpd_state *mpd_state, const int song_id);
static bool mpd_client_jukebox(t_config *config, t_mpd_state *mpd_state);
static bool mpd_client_smartpls_save(t_config *config, t_mpd_state *mpd_state, const char *smartpltype, const char *playlist, const char *tag, const char *searchstr, const int maxentries, const int timerange);
static int mpd_client_smartpls_put(t_config *config, char *buffer, const char *playlist);
static bool mpd_client_smartpls_update_all(t_config *config, t_mpd_state *mpd_state);
static bool mpd_client_smartpls_clear(t_mpd_state *mpd_state, const char *playlist);
static bool mpd_client_smartpls_update_sticker(t_mpd_state *mpd_state, const char *playlist, const char *sticker, const int maxentries);
static bool mpd_client_smartpls_update_newest(t_config *config, t_mpd_state *mpd_state, const char *playlist, const int timerange);
static bool mpd_client_smartpls_update_search(t_config *config, t_mpd_state *mpd_state, const char *playlist, const char *tag, const char *searchstr);
static int mpd_client_get_updatedb_state(t_mpd_state *mpd_state, char *buffer);
static int mpd_client_put_state(t_mpd_state *mpd_state, char *buffer);
static int mpd_client_put_outputs(t_mpd_state *mpd_state, char *buffer);
static int mpd_client_put_current_song(t_config *config, t_mpd_state *mpd_state, char *buffer);
static int mpd_client_put_queue(t_config *config, t_mpd_state *mpd_state, char *buffer, const unsigned int offset);
static int mpd_client_put_browse(t_config *config, t_mpd_state *mpd_state, char *buffer, const char *path, const unsigned int offset, const char *filter);
static int mpd_client_search(t_config *config, t_mpd_state *mpd_state, char *buffer, const char *searchstr, const char *filter, const char *plist, const unsigned int offset);
static int mpd_client_search_adv(t_config *config, t_mpd_state *mpd_state, char *buffer, const char *expression, const char *sort, const bool sortdesc, const char *grouptag, const char *plist, const unsigned int offset);
static int mpd_client_search_queue(t_config *config, t_mpd_state *mpd_state, char *buffer, const char *mpdtagtype, const unsigned int offset, const char *searchstr);
static int mpd_client_put_volume(t_mpd_state *mpd_state, char *buffer);
static int mpd_client_put_stats(t_mpd_state *mpd_state, char *buffer);
static int mpd_client_put_settings(t_mpd_state *mpd_state, char *buffer);
static int mpd_client_put_db_tag(t_config *config, t_mpd_state *mpd_state, char *buffer, const unsigned int offset, const char *mpdtagtype, const char *mpdsearchtagtype, const char *searchstr, const char *filter);
static int mpd_client_put_songs_in_album(t_config *config, t_mpd_state *mpd_state, char *buffer, const char *album, const char *search, const char *tag);
static int mpd_client_put_playlists(t_config *config, t_mpd_state *mpd_state, char *buffer, const unsigned int offset, const char *filter);
static int mpd_client_put_playlist_list(t_config *config, t_mpd_state *mpd_state, char *buffer, const char *uri, const unsigned int offset, const char *filter);
static int mpd_client_put_songdetails(t_config *config, t_mpd_state *mpd_state, char *buffer, const char *uri);
static int mpd_client_put_last_played_songs(t_config *config, t_mpd_state *mpd_state, char *buffer, const unsigned int offset);
static int mpd_client_queue_crop(t_mpd_state *mpd_state, char *buffer);
static void mpd_client_disconnect(t_config *config, t_mpd_state *mpd_state);
static int mpd_client_read_last_played(t_config *config, t_mpd_state *mpd_state);

//public functions
void *mpd_client_loop(void *arg_config) {
    t_config *config = (t_config *) arg_config;
    //State of mpd connection
    t_mpd_state mpd_state;
    mpd_state.conn_state = MPD_DISCONNECTED;
    mpd_state.timeout = 3000;
    mpd_state.song_id = -1;
    mpd_state.next_song_id = -1;
    mpd_state.last_song_id = -1;
    mpd_state.queue_version = 0;
    mpd_state.queue_length = 0;
    mpd_state.last_update_sticker_song_id = -1;
    mpd_state.last_last_played_id = -1;
    mpd_state.feat_sticker = false;
    mpd_state.feat_playlists = false;
    mpd_state.feat_tags = false;
    mpd_state.feat_library = false;
    mpd_state.feat_advsearch = false;
    mpd_state.feat_smartpls = false;
    mpd_state.jukeboxMode = JUKEBOX_OFF;
    mpd_state.jukeboxPlaylist = strdup("Database");
    mpd_state.jukeboxQueueLength = 1;
    mpd_state.autoPlay = false;
    list_init(&mpd_state.mpd_tags);
    list_init(&mpd_state.mympd_tags);
    list_init(&mpd_state.mympd_searchtags);
    list_init(&mpd_state.mympd_browsetags);

    char testdirname[400];    
    snprintf(testdirname, 400, "%s/library", DOC_ROOT);
    if (testdir("Link to mpd music_directory", testdirname)) {
        LOG_INFO() printf("Enabling featLibrary support\n");
        mpd_state.feat_library = true;
    }

    //read last played songs history file
    list_init(&mpd_state.last_played);
    int len = mpd_client_read_last_played(config, &mpd_state);
    LOG_INFO() printf("Reading last played songs: %d\n", len);
    
    while (s_signal_received == 0) {
        mpd_client_idle(config, &mpd_state);
    }
    //Cleanup
    mpd_client_disconnect(config, &mpd_state);
    list_free(&mpd_state.mpd_tags);
    list_free(&mpd_state.mympd_tags);
    list_free(&mpd_state.mympd_searchtags);
    list_free(&mpd_state.mympd_browsetags);
    list_free(&mpd_state.last_played);
    free(mpd_state.jukeboxPlaylist);
    return NULL;
}

//private functions
static void mpd_client_api(t_config *config, t_mpd_state *mpd_state, void *arg_request) {
    t_work_request *request = (t_work_request*) arg_request;
    char buffer[MAX_SIZE];
    size_t len = 0;
    unsigned int uint_buf1, uint_buf2, uint_rc;
    int je, int_buf1, int_rc; 
    float float_buf;
    bool bool_buf, rc;
    char *p_charbuf1, *p_charbuf2, *p_charbuf3, *p_charbuf4;

    LOG_VERBOSE() printf("API request (%ld): %.*s\n", request->conn_id, request->length, request->data);
    
    switch(request->cmd_id) {
        case MPD_API_LIKE:
            if (mpd_state->feat_sticker) {
                je = json_scanf(request->data, request->length, "{data: {uri: %Q, like: %d}}", &p_charbuf1, &uint_buf1);
                if (je == 2) {        
                    if (!mpd_client_like_song_uri(mpd_state, p_charbuf1, uint_buf1))
                        len = snprintf(buffer, MAX_SIZE, "{\"type\": \"error\", \"data\": \"Can't set like.\"}");
                    else
                        len = snprintf(buffer, MAX_SIZE, "{\"type\": \"result\", \"data\": \"ok\"}");
                    free(p_charbuf1);
                }
            } 
            else {
                len = snprintf(buffer, MAX_SIZE, "{\"type\": \"error\", \"data\": \"MPD stickers are disabled\"}");
                printf("MPD_API_LIKE: MPD stickers are disabled\n");
            }
            break;
        case MPD_API_PLAYER_STATE:
            len = mpd_client_put_state(mpd_state, buffer);
            break;
        case MYMPD_API_SETTINGS_SET:
            //only update mpd_state, already saved in mympd_api.c
            je = json_scanf(request->data, request->length, "{data: {jukeboxMode: %d}}", &mpd_state->jukeboxMode);
            je = json_scanf(request->data, request->length, "{data: {jukeboxPlaylist: %Q}}", &p_charbuf1);
            if (je == 1) {
                free(mpd_state->jukeboxPlaylist);
                mpd_state->jukeboxPlaylist = p_charbuf1;
                p_charbuf1 = NULL;
            }
            je = json_scanf(request->data, request->length, "{data: {jukeboxQueueLength: %d}}", &mpd_state->jukeboxQueueLength);
            je = json_scanf(request->data, request->length, "{data: {autoPlay: %B}}", &mpd_state->autoPlay);
            //set mpd options
            je = json_scanf(request->data, request->length, "{data: {random: %u}}", &uint_buf1);
            if (je == 1) {
                if (!mpd_run_random(mpd_state->conn, uint_buf1))
                    len = snprintf(buffer, MAX_SIZE, "{\"type\": \"error\", \"data\": \"Can't set mpd state random.\"}");
            }
            je = json_scanf(request->data, request->length, "{data: {repeat: %u}}", &uint_buf1);
            if (je == 1) {
                if (!mpd_run_repeat(mpd_state->conn, uint_buf1))
                    len = snprintf(buffer, MAX_SIZE, "{\"type\": \"error\", \"data\": \"Can't set mpd state repeat.\"}");
            }
            je = json_scanf(request->data, request->length, "{data: {consume: %u}}", &uint_buf1);
            if (je == 1) {
                if (!mpd_run_consume(mpd_state->conn, uint_buf1))
                    len = snprintf(buffer, MAX_SIZE, "{\"type\": \"error\", \"data\": \"Can't set mpd state consume.\"}");
            }
            je = json_scanf(request->data, request->length, "{data: {single: %u}}", &uint_buf1);
            if (je == 1) {
                if (!mpd_run_single(mpd_state->conn, uint_buf1))
                    len = snprintf(buffer, MAX_SIZE, "{\"type\": \"error\", \"data\": \"Can't set mpd state single.\"}");
            }
            je = json_scanf(request->data, request->length, "{data: {crossfade: %u}}", &uint_buf1);
            if (je == 1) {
                if (!mpd_run_crossfade(mpd_state->conn, uint_buf1))
                    len = snprintf(buffer, MAX_SIZE, "{\"type\": \"error\", \"data\": \"Can't set mpd state crossfade.\"}");
            }
            if (config->mixramp) {
                je = json_scanf(request->data, request->length, "{data: {mixrampdb: %f}}", &float_buf);
                if (je == 1) {
                    if (!mpd_run_mixrampdb(mpd_state->conn, float_buf))
                        len = snprintf(buffer, MAX_SIZE, "{\"type\": \"error\", \"data\": \"Can't set mpd state mixrampdb.\"}");
                }
                je = json_scanf(request->data, request->length, "{data: {mixrampdelay: %f}}", &float_buf);
                if (je == 1) {
                    if (!mpd_run_mixrampdelay(mpd_state->conn, float_buf))
                        len = snprintf(buffer, MAX_SIZE, "{\"type\": \"error\", \"data\": \"Can't set mpd state mixrampdelay.\"}");
                }
            }
            
            je = json_scanf(request->data, request->length, "{data: {replaygain: %Q}}", &p_charbuf1);
            if (je == 1) {
                if (!mpd_send_command(mpd_state->conn, "replay_gain_mode", p_charbuf1, NULL))
                    len = snprintf(buffer, MAX_SIZE, "{\"type\": \"error\", \"data\": \"Can't set mpd state replaygain.\"}");
                mpd_response_finish(mpd_state->conn);
                free(p_charbuf1);            
            }
            if (mpd_state->jukeboxMode != JUKEBOX_OFF) {
                mpd_client_jukebox(config, mpd_state);
            }
            if (len == 0)
                len = snprintf(buffer, MAX_SIZE, "{\"type\": \"result\", \"data\": \"ok\"}");
            break;
        case MPD_API_DATABASE_UPDATE:
            uint_rc = mpd_run_update(mpd_state->conn, NULL);
            if (uint_rc > 0)
                len = snprintf(buffer, MAX_SIZE, "{\"type\": \"result\", \"data\": \"ok\"}");
            break;
        case MPD_API_DATABASE_RESCAN:
            uint_rc = mpd_run_rescan(mpd_state->conn, NULL);
            if (uint_rc > 0)
                len = snprintf(buffer, MAX_SIZE, "{\"type\": \"result\", \"data\": \"ok\"}");
            break;
        case MPD_API_SMARTPLS_UPDATE_ALL:
            rc = mpd_client_smartpls_update_all(config, mpd_state);
            if (rc == true)
                len = snprintf(buffer, MAX_SIZE, "{\"type\": \"result\", \"data\": \"Smart Playlists updated\"}");
            else
                len = snprintf(buffer, MAX_SIZE, "{\"type\": \"error\", \"data\": \"Smart Playlists update failed\"}");
            break;
        case MPD_API_SMARTPLS_SAVE:
            je = json_scanf(request->data, request->length, "{data: {type: %Q}}", &p_charbuf1);
            len = 0;
            rc = false;
            if (je == 1) {
                if (strcmp(p_charbuf1, "sticker") == 0) {
                    je = json_scanf(request->data, request->length, "{data: {playlist: %Q, sticker: %Q, maxentries: %d}}", &p_charbuf2, &p_charbuf3, &int_buf1);
                    if (je == 3) {
                        rc = mpd_client_smartpls_save(config, mpd_state, p_charbuf1, p_charbuf2, p_charbuf3, NULL, int_buf1, 0);
                        free(p_charbuf2);
                        free(p_charbuf3);
                    }
                }
                else if (strcmp(p_charbuf1, "newest") == 0) {
                    je = json_scanf(request->data, request->length, "{data: {playlist: %Q, timerange: %d}}", &p_charbuf2, &int_buf1);
                    if (je == 2) {
                        rc = mpd_client_smartpls_save(config, mpd_state, p_charbuf1, p_charbuf2, NULL, NULL, 0, int_buf1);
                        free(p_charbuf2);
                    }
                }            
                else if (strcmp(p_charbuf1, "search") == 0) {
                    je = json_scanf(request->data, request->length, "{data: {playlist: %Q, tag: %Q, searchstr: %Q}}", &p_charbuf2, &p_charbuf3, &p_charbuf4);
                    if (je == 3) {
                        rc = mpd_client_smartpls_save(config, mpd_state, p_charbuf1, p_charbuf2, p_charbuf3, p_charbuf4, 0, 0);
                        free(p_charbuf2);
                        free(p_charbuf3);                    
                        free(p_charbuf4);
                    }
                }
                free(p_charbuf1);
            }
            if (rc == true) {
                len = snprintf(buffer, MAX_SIZE, "{\"type\": \"result\", \"data\": \"ok\"}");
            }
            else {
                len = snprintf(buffer, MAX_SIZE, "{\"type\": \"error\", \"data\": \"Saving playlist failed\"}");
            }
            break;
        case MPD_API_SMARTPLS_GET:
            je = json_scanf(request->data, request->length, "{data: {playlist: %Q}}", &p_charbuf1);
            if (je == 1) {
                len = mpd_client_smartpls_put(config, buffer, p_charbuf1);
                free(p_charbuf1);
            }
            break;
        case MPD_API_PLAYER_PAUSE:
            if (mpd_run_toggle_pause(mpd_state->conn))
                len = snprintf(buffer, MAX_SIZE, "{\"type\": \"result\", \"data\": \"ok\"}");
            else {
                len = snprintf(buffer, MAX_SIZE, "{\"type\": \"error\", \"data\": \"Toggling player pause failed.\"}");
                printf("MPD_API_PLAYER_PAUSE: Error mpd_run_toggle_pause()\n");
            }
            break;
        case MPD_API_PLAYER_PREV:
            if (mpd_run_previous(mpd_state->conn))
                len = snprintf(buffer, MAX_SIZE, "{\"type\": \"result\", \"data\": \"ok\"}");
            else {
                len = snprintf(buffer, MAX_SIZE, "{\"type\": \"error\", \"data\": \"Goto previous song failed.\"}");
                printf("MPD_API_PLAYER_PREV: Error mpd_run_previous()\n");
            }
            break;
        case MPD_API_PLAYER_NEXT:
            if (mpd_state->feat_sticker)
                mpd_client_count_song_id(mpd_state, mpd_state->song_id, "skipCount", 1);
            if (mpd_run_next(mpd_state->conn))
                len = snprintf(buffer, MAX_SIZE, "{\"type\": \"result\", \"data\": \"ok\"}");
            else {
                len = snprintf(buffer, MAX_SIZE, "{\"type\": \"error\", \"data\": \"Skip to next song failed.\"}");
                printf("MPD_API_PLAYER_NEXT: Error mpd_run_next()\n");
            }
            break;
        case MPD_API_PLAYER_PLAY:
            if (mpd_run_play(mpd_state->conn))
                len = snprintf(buffer, MAX_SIZE, "{\"type\": \"result\", \"data\": \"ok\"}");
            else {
                len = snprintf(buffer, MAX_SIZE, "{\"type\": \"error\", \"data\": \"Begin to play failed.\"}");
                printf("MPD_API_PLAYER_PLAY: Error mpd_run_play()\n");
            }
            break;
        case MPD_API_PLAYER_STOP:
            if (mpd_run_stop(mpd_state->conn))
                len = snprintf(buffer, MAX_SIZE, "{\"type\": \"result\", \"data\": \"ok\"}");
            else {
                len = snprintf(buffer, MAX_SIZE, "{\"type\": \"error\", \"data\": \"Stopping player failed.\"}");
                printf("MPD_API_PLAYER_STOP: Error mpd_run_stop()\n");
            }
            break;
        case MPD_API_QUEUE_CLEAR:
            if (mpd_run_clear(mpd_state->conn))
                len = snprintf(buffer, MAX_SIZE, "{\"type\": \"result\", \"data\": \"ok\"}");
            else {
                len = snprintf(buffer, MAX_SIZE, "{\"type\": \"error\", \"data\": \"Clearing playlist failed.\"}");
                printf("MPD_API_QUEUE_CLEAR: Error mpd_run_clear()\n");
            }
            break;
        case MPD_API_QUEUE_CROP:
            len = mpd_client_queue_crop(mpd_state, buffer);
            break;
        case MPD_API_QUEUE_RM_TRACK:
            je = json_scanf(request->data, request->length, "{data: {track:%u}}", &uint_buf1);
            if (je == 1) {
                if (mpd_run_delete_id(mpd_state->conn, uint_buf1))
                    len = snprintf(buffer, MAX_SIZE, "{\"type\": \"result\", \"data\": \"ok\"}");
                else {
                    len = snprintf(buffer, MAX_SIZE, "{\"type\": \"error\", \"data\": \"Removing track from queue failed.\"}");
                    printf("MPD_API_QUEUE_RM_TRACK: Error mpd_run_delete_id()\n");
                }
            }
            break;
        case MPD_API_QUEUE_RM_RANGE:
            je = json_scanf(request->data, request->length, "{data: {start: %u, end: %u}}", &uint_buf1, &uint_buf2);
            if (je == 2) {
                if (mpd_run_delete_range(mpd_state->conn, uint_buf1, uint_buf2))
                    len = snprintf(buffer, MAX_SIZE, "{\"type\": \"result\", \"data\": \"ok\"}");
                else {
                    len = snprintf(buffer, MAX_SIZE, "{\"type\": \"error\", \"data\": \"Removing track range from queue failed.\"}");
                    printf("MPD_API_QUEUE_RM_RANGE: Error mpd_run_delete_range()\n");
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
                    len = snprintf(buffer, MAX_SIZE, "{\"type\": \"result\", \"data\": \"ok\"}");
                else {
                    len = snprintf(buffer, MAX_SIZE, "{\"type\": \"error\", \"data\": \"Moving track in queue failed.\"}");
                    printf("MPD_API_QUEUE_MOVE_TRACK: Error mpd_run_move()\n");
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
                    len = snprintf(buffer, MAX_SIZE, "{\"type\": \"result\", \"data\": \"ok\"}");
                }
                else {
                    len = snprintf(buffer, MAX_SIZE, "{\"type\": \"error\", \"data\": \"Moving track in playlist failed.\"}");
                    printf("MPD_API_PLAYLIST_MOVE_TRACK: Error mpd_send_playlist_move()\n");
                }
                free(p_charbuf1);
            }
            break;
        case MPD_API_PLAYER_PLAY_TRACK:
            je = json_scanf(request->data, request->length, "{data: { track:%u}}", &uint_buf1);
            if (je == 1) {
                if (mpd_run_play_id(mpd_state->conn, uint_buf1))
                    len = snprintf(buffer, MAX_SIZE, "{\"type\": \"result\", \"data\": \"ok\"}");
                else {
                    len = snprintf(buffer, MAX_SIZE, "{\"type\": \"error\", \"data\": \"Set playing track failed.\"}");
                    printf("MPD_API_PLAYER_PLAY_TRACK: Error mpd_run_play_id()\n");
                }
            }
            break;
        case MPD_API_PLAYER_OUTPUT_LIST:
            len = mpd_client_put_outputs(mpd_state, buffer);
            break;
        case MPD_API_PLAYER_TOGGLE_OUTPUT:
            je = json_scanf(request->data, request->length, "{data: {output: %u, state: %u}}", &uint_buf1, &uint_buf2);
            if (je == 2) {
                if (uint_buf2) {
                    if (mpd_run_enable_output(mpd_state->conn, uint_buf1))
                        len = snprintf(buffer, MAX_SIZE, "{\"type\": \"result\", \"data\": \"ok\"}");
                    else {
                        len = snprintf(buffer, MAX_SIZE, "{\"type\": \"error\", \"data\": \"Enabling output failed.\"}");
                        printf("MPD_API_PLAYER_TOGGLE_OUTPUT: Error mpd_run_enable_output()\n");
                    }
                }
                else {
                    if (mpd_run_disable_output(mpd_state->conn, uint_buf1))
                        len = snprintf(buffer, MAX_SIZE, "{\"type\": \"result\", \"data\": \"ok\"}");
                    else {
                        len = snprintf(buffer, MAX_SIZE, "{\"type\": \"error\", \"data\": \"Disabling output failed.\"}");
                        printf("MPD_API_PLAYER_TOGGLE_OUTPUT: Error mpd_run_disable_output()\n");
                    }
                }
            }
            break;
        case MPD_API_PLAYER_VOLUME_SET:
            je = json_scanf(request->data, request->length, "{data: {volume:%u}}", &uint_buf1);
            if (je == 1) {
                if (mpd_run_set_volume(mpd_state->conn, uint_buf1))
                    len = snprintf(buffer, MAX_SIZE, "{\"type\": \"result\", \"data\": \"ok\"}");
                else {
                    len = snprintf(buffer, MAX_SIZE, "{\"type\": \"error\", \"data\": \"Setting volume failed.\"}");
                    printf("MPD_API_PLAYER_PLAY_TRACK: Error mpd_run_set_volume()\n");
                }
            }
            break;
        case MPD_API_PLAYER_VOLUME_GET:
            len = mpd_client_put_volume(mpd_state, buffer);
            break;            
        case MPD_API_PLAYER_SEEK:
            je = json_scanf(request->data, request->length, "{data: {songid: %u, seek: %u}}", &uint_buf1, &uint_buf2);
            if (je == 2) {
                if (mpd_run_seek_id(mpd_state->conn, uint_buf1, uint_buf2))
                    len = snprintf(buffer, MAX_SIZE, "{\"type\": \"result\", \"data\": \"ok\"}");
                else {
                    len = snprintf(buffer, MAX_SIZE, "{\"type\": \"error\", \"data\": \"Seeking song failed.\"}");
                    printf("MPD_API_PLAYER_SEEK: Error mpd_run_seek_id()\n");
                }
            }
            break;
        case MPD_API_QUEUE_LIST:
            je = json_scanf(request->data, request->length, "{data: {offset: %u}}", &uint_buf1);
            if (je == 1) {
                len = mpd_client_put_queue(config, mpd_state, buffer, uint_buf1);
            }
            break;
        case MPD_API_QUEUE_LAST_PLAYED:
            je = json_scanf(request->data, request->length, "{data: {offset: %u}}", &uint_buf1);
            if (je == 1) {
                len = mpd_client_put_last_played_songs(config, mpd_state, buffer, uint_buf1);
            }
            break;
        case MPD_API_PLAYER_CURRENT_SONG:
                len = mpd_client_put_current_song(config, mpd_state, buffer);
            break;
        case MPD_API_DATABASE_SONGDETAILS:
            je = json_scanf(request->data, request->length, "{data: { uri: %Q}}", &p_charbuf1);
            if (je == 1) {
                len = mpd_client_put_songdetails(config, mpd_state, buffer, p_charbuf1);
                free(p_charbuf1);
            }
            break;            
        case MPD_API_DATABASE_TAG_LIST:
            je = json_scanf(request->data, request->length, "{data: {offset: %u, filter: %Q, tag: %Q}}", &uint_buf1, &p_charbuf1, &p_charbuf2);
            if (je == 3) {
                len = mpd_client_put_db_tag(config, mpd_state, buffer, uint_buf1, p_charbuf2, "", "", p_charbuf1);
                free(p_charbuf1);
                free(p_charbuf2);
            }
            break;
        case MPD_API_DATABASE_TAG_ALBUM_LIST:
            je = json_scanf(request->data, request->length, "{data: {offset: %u, filter: %Q, search: %Q, tag: %Q}}", &uint_buf1, &p_charbuf1, &p_charbuf2, &p_charbuf3);
            if (je == 4) {
                len = mpd_client_put_db_tag(config, mpd_state, buffer, uint_buf1, "Album", p_charbuf3, p_charbuf2, p_charbuf1);
                free(p_charbuf1);
                free(p_charbuf2);
                free(p_charbuf3);
            }
            break;
        case MPD_API_DATABASE_TAG_ALBUM_TITLE_LIST:
            je = json_scanf(request->data, request->length, "{data: {album: %Q, search: %Q, tag: %Q}}", &p_charbuf1, &p_charbuf2, &p_charbuf3);
            if (je == 3) {
                len = mpd_client_put_songs_in_album(config, mpd_state, buffer, p_charbuf1, p_charbuf2, p_charbuf3);
                free(p_charbuf1);
                free(p_charbuf2);
                free(p_charbuf3);
            } 
            break;
        case MPD_API_PLAYLIST_RENAME:
            je = json_scanf(request->data, request->length, "{data: {from: %Q, to: %Q}}", &p_charbuf1, &p_charbuf2);
            if (je == 2) {
                //rename smart playlist
                char old_pl_file[400];
                char new_pl_file[400];
                if (validate_string(p_charbuf1) == false || validate_string(p_charbuf2) == false) {
                    len = snprintf(buffer, MAX_SIZE, "{\"type\": \"error\", \"data\": \"Invalid filename.\"}");
                    break;
                }
                snprintf(old_pl_file, 400, "%s/smartpls/%s", config->varlibdir, p_charbuf1);
                snprintf(new_pl_file, 400, "%s/smartpls/%s", config->varlibdir, p_charbuf2);
                if (access(old_pl_file, F_OK ) != -1) {
                    if (access(new_pl_file, F_OK ) == -1) {
                        if (rename(old_pl_file, new_pl_file) == -1) {
                            len = snprintf(buffer, MAX_SIZE, "{\"type\": \"error\", \"data\": \"Renaming playlist failed.\"}");
                            printf("MPD_API_PLAYLIST_RENAME: Rename failed()\n");
                        }
                        //rename mpd playlist
                        else if (mpd_run_rename(mpd_state->conn, p_charbuf1, p_charbuf2))
                            len = snprintf(buffer, MAX_SIZE, "{\"type\": \"result\", \"data\": \"Renamed playlist %s to %s\"}", p_charbuf1, p_charbuf2);
                        else {
                            len = snprintf(buffer, MAX_SIZE, "{\"type\": \"error\", \"data\": \"Renaming playlist failed.\"}");
                            printf("MPD_API_PLAYLIST_RENAME: Error mpd_run_rename()\n");
                        }
                    } else 
                        len = snprintf(buffer, MAX_SIZE, "{\"type\": \"error\", \"data\": \"Destination playlist %s already exists\"}", p_charbuf2);
                }
                else {
                    if (mpd_run_rename(mpd_state->conn, p_charbuf1, p_charbuf2))
                        len = snprintf(buffer, MAX_SIZE, "{\"type\": \"result\", \"data\": \"Renamed playlist %s to %s\"}", p_charbuf1, p_charbuf2);
                    else {
                        len = snprintf(buffer, MAX_SIZE, "{\"type\": \"error\", \"data\": \"Renaming playlist failed.\"}");
                        printf("MPD_API_PLAYLIST_RENAME: Error mpd_run_rename()\n");
                    }
                }
                free(p_charbuf1);
                free(p_charbuf2);
            }
            break;            
        case MPD_API_PLAYLIST_LIST:
            je = json_scanf(request->data, request->length, "{data: {offset: %u, filter: %Q}}", &uint_buf1, &p_charbuf1);
            if (je == 2) {
                len = mpd_client_put_playlists(config, mpd_state, buffer, uint_buf1, p_charbuf1);
                free(p_charbuf1);
            }
            break;
        case MPD_API_PLAYLIST_CONTENT_LIST:
            je = json_scanf(request->data, request->length, "{data: {uri: %Q, offset:%u, filter:%Q}}", &p_charbuf1, &uint_buf1, &p_charbuf2);
            if (je == 3) {
                len = mpd_client_put_playlist_list(config, mpd_state, buffer, p_charbuf1, uint_buf1, p_charbuf2);
                free(p_charbuf1);
                free(p_charbuf2);
            }
            break;
        case MPD_API_PLAYLIST_ADD_TRACK:
            je = json_scanf(request->data, request->length, "{data: {plist:%Q, uri:%Q}}", &p_charbuf1, &p_charbuf2);
            if (je == 2) {
                if (mpd_run_playlist_add(mpd_state->conn, p_charbuf1, p_charbuf2))
                    len = snprintf(buffer, MAX_SIZE, "{\"type\": \"result\", \"data\": \"Added %s to playlist %s\"}", p_charbuf2, p_charbuf1);
                else {
                    len = snprintf(buffer, MAX_SIZE, "{\"type\": \"error\", \"data\": \"Adding song to playlist failed.\"}");
                    printf("MPD_API_PLAYLIST_ADD_TRACK: Error mpd_run_playlist_add()\n");
                }
                free(p_charbuf1);
                free(p_charbuf2);                
            }
            break;
        case MPD_API_PLAYLIST_CLEAR:
            je = json_scanf(request->data, request->length, "{data: {uri:%Q}}", &p_charbuf1);
            if (je == 1) {
                if (mpd_run_playlist_clear(mpd_state->conn, p_charbuf1))
                    len = snprintf(buffer, MAX_SIZE, "{\"type\": \"result\", \"data\": \"ok\"}");
                else {
                    len = snprintf(buffer, MAX_SIZE, "{\"type\": \"error\", \"data\": \"Clearing playlist failed.\"}");
                    printf("MPD_API_PLAYLIST_CLEAR: Error mpd_run_playlist_clear()\n");
                }
                free(p_charbuf1);
            }
            break;
        case MPD_API_PLAYLIST_RM_TRACK:
            je = json_scanf(request->data, request->length, "{data: {uri:%Q, track:%u}}", &p_charbuf1, &uint_buf1);
            if (je == 2) {
                if (mpd_run_playlist_delete(mpd_state->conn, p_charbuf1, uint_buf1))
                    len = snprintf(buffer, MAX_SIZE, "{\"type\": \"result\", \"data\": \"ok\"}");
                else {
                    len = snprintf(buffer, MAX_SIZE, "{\"type\": \"error\", \"data\": \"Removing track from playlist failed.\"}");
                    printf("MPD_API_PLAYLIST_RM_TRACK: Error mpd_run_playlist_delete()\n");
                }
                free(p_charbuf1);
            }
            break;
        case MPD_API_DATABASE_FILESYSTEM_LIST:
            je = json_scanf(request->data, request->length, "{data: {offset:%u, filter:%Q, path:%Q}}", &uint_buf1, &p_charbuf1, &p_charbuf2);
            if (je == 3) {
                len = mpd_client_put_browse(config, mpd_state, buffer, p_charbuf2, uint_buf1, p_charbuf1);
                free(p_charbuf1);
                free(p_charbuf2);
            }
            break;
        case MPD_API_QUEUE_ADD_TRACK_AFTER:
            je = json_scanf(request->data, request->length, "{data: {uri:%Q, to:%d}}", &p_charbuf1, &int_buf1);
            if (je == 2) {
                int_rc = mpd_run_add_id_to(mpd_state->conn, p_charbuf1, int_buf1);
                if (int_rc > -1 ) 
                    len = snprintf(buffer, MAX_SIZE, "{\"type\": \"result\", \"data\": \"ok\"}");
                else {
                    len = snprintf(buffer, MAX_SIZE, "{\"type\": \"error\", \"data\": \"Adding track to queue failed.\"}");
                    printf("MPD_API_QUEUE_ADD_TRACK_AFTER: Error mpd_run_add_id_to()\n");
                }
                free(p_charbuf1);
            }
            break;
        case MPD_API_QUEUE_REPLACE_TRACK:
            je = json_scanf(request->data, request->length, "{data: {uri:%Q }}", &p_charbuf1);
            if (je == 1) {
                if (!mpd_run_clear(mpd_state->conn)) {
                    len = snprintf(buffer, MAX_SIZE, "{\"type\": \"error\", \"data\": \"Clearing queue failed.\"}");
                    printf("MPD_API_QUEUE_REPLACE_TRACK: Error mpd_run_add_id_to()\n");                
                }
                else if (!mpd_run_add(mpd_state->conn, p_charbuf1)) {
                    len = snprintf(buffer, MAX_SIZE, "{\"type\": \"error\", \"data\": \"Adding track to queue failed.\"}");
                    printf("MPD_API_QUEUE_REPLACE_TRACK: Error mpd_run_add_id_to()\n");                
                }
                else if (!mpd_run_play(mpd_state->conn)) {
                    len = snprintf(buffer, MAX_SIZE, "{\"type\": \"error\", \"data\": \"Playing failed.\"}");
                    printf("MPD_API_QUEUE_REPLACE_TRACK: Error mpd_run_play()\n");                
                }
                else
                    len = snprintf(buffer, MAX_SIZE, "{\"type\": \"result\", \"data\": \"ok\"}");
                free(p_charbuf1);
            }
            break;
        case MPD_API_QUEUE_ADD_TRACK:
            je = json_scanf(request->data, request->length, "{data: {uri:%Q}}", &p_charbuf1);
            if (je == 1) {
                if (mpd_run_add(mpd_state->conn, p_charbuf1))
                    len = snprintf(buffer, MAX_SIZE, "{\"type\": \"result\", \"data\": \"ok\"}");
                else {
                    len = snprintf(buffer, MAX_SIZE, "{\"type\": \"error\", \"data\": \"Append track to queue failed.\"}");
                    printf("MPD_API_QUEUE_ADD_TRACK: Error mpd_run_add()\n");
                }
                free(p_charbuf1);
            }
            break;
        case MPD_API_QUEUE_ADD_PLAY_TRACK:
            je = json_scanf(request->data, request->length, "{data: {uri:%Q}}", &p_charbuf1);
            if (je == 1) {
                int_buf1 = mpd_run_add_id(mpd_state->conn, p_charbuf1);
                if (int_buf1 != -1) {
                    if (mpd_run_play_id(mpd_state->conn, int_buf1))
                        len = snprintf(buffer, MAX_SIZE, "{\"type\": \"result\", \"data\": \"ok\"}");
                    else {
                        len = snprintf(buffer, MAX_SIZE, "{\"type\": \"error\", \"data\": \"Setting playstate failed.\"}");
                        printf("MPD_API_QUEUE_ADD_PLAY_TRACK: Error mpd_run_play_id()\n");
                    }
                }
                else {
                    len = snprintf(buffer, MAX_SIZE, "{\"type\": \"error\", \"data\": \"Adding track to queue failed.\"}");
                    printf("MPD_API_QUEUE_ADD_PLAY_TRACK: Error mpd_run_add_id()\n");
                }
                free(p_charbuf1);
            }
            break;
        case MPD_API_QUEUE_REPLACE_PLAYLIST:
            je = json_scanf(request->data, request->length, "{data: {plist:%Q}}", &p_charbuf1);
            if (je == 1) {
                if (!mpd_run_clear(mpd_state->conn)) {
                    len = snprintf(buffer, MAX_SIZE, "{\"type\": \"error\", \"data\": \"Clearing queue failed.\"}");
                    printf("MPD_API_QUEUE_REPLACE_PLAYLIST: Error mpd_run_clear()\n");                
                }
                else if (!mpd_run_load(mpd_state->conn, p_charbuf1)) {
                    len = snprintf(buffer, MAX_SIZE, "{\"type\": \"error\", \"data\": \"Adding playlist to queue failed.\"}");
                    printf("MPD_API_QUEUE_REPLACE_PLAYLIST: Error mpd_run_load()\n");
                }
                else if (!mpd_run_play(mpd_state->conn)) {
                    len = snprintf(buffer, MAX_SIZE, "{\"type\": \"error\", \"data\": \"Setting playstate failed.\"}");
                    printf("MPD_API_QUEUE_REPLACE_PLAYLIST: Error mpd_run_play()\n");
                }
                else
                    len = snprintf(buffer, MAX_SIZE, "{\"type\": \"result\", \"data\": \"ok\"}");
                free(p_charbuf1);
            }
            break;
        case MPD_API_QUEUE_ADD_PLAYLIST:
            je = json_scanf(request->data, request->length, "{data: {plist:%Q}}", &p_charbuf1);
            if (je == 1) {
                if (mpd_run_load(mpd_state->conn, p_charbuf1))
                    len = snprintf(buffer, MAX_SIZE, "{\"type\": \"result\", \"data\": \"ok\"}");
                else {
                    len = snprintf(buffer, MAX_SIZE, "{\"type\": \"error\", \"data\": \"Adding playlist to queue failed.\"}");
                    printf("MPD_API_QUEUE_ADD_PLAYLIST: Error mpd_run_add_id()\n");
                }
                free(p_charbuf1);
            }
            break;
        case MPD_API_QUEUE_SAVE:
            je = json_scanf(request->data, request->length, "{ data: {plist:%Q}}", &p_charbuf1);
            if (je == 1) {
                if (mpd_run_save(mpd_state->conn, p_charbuf1))
                    len = snprintf(buffer, MAX_SIZE, "{\"type\": \"result\", \"data\": \"ok\"}");
                else {
                    len = snprintf(buffer, MAX_SIZE, "{\"type\": \"error\", \"data\": \"Saving queue as playlist failed.\"}");
                    printf("MPD_API_QUEUE_SAVE: Error mpd_run_save()\n");
                }
                free(p_charbuf1);
            }
            break;
        case MPD_API_QUEUE_SEARCH:
            je = json_scanf(request->data, request->length, "{data: {offset:%u, filter:%Q, searchstr:%Q}}", &uint_buf1, &p_charbuf1, &p_charbuf2);
            if (je == 3) {
                len = mpd_client_search_queue(config, mpd_state, buffer, p_charbuf1, uint_buf1, p_charbuf2);
                free(p_charbuf1);
                free(p_charbuf2);
            }
            break;            
        case MPD_API_DATABASE_SEARCH:
            je = json_scanf(request->data, request->length, "{data: {searchstr:%Q, filter:%Q, plist:%Q, offset:%u}}", &p_charbuf1, &p_charbuf2, &p_charbuf3, &uint_buf1);
            if (je == 4) {
                len = mpd_client_search(config, mpd_state, buffer, p_charbuf1, p_charbuf2, p_charbuf3, uint_buf1);
                free(p_charbuf1);
                free(p_charbuf2);
                free(p_charbuf3);
            }
            break;
        case MPD_API_DATABASE_SEARCH_ADV:
            je = json_scanf(request->data, request->length, "{data: {expression:%Q, sort:%Q, sortdesc:%B, plist:%Q, offset:%u}}", 
                &p_charbuf1, &p_charbuf2, &bool_buf, &p_charbuf3, &uint_buf1);
            if (je == 5) {
                len = mpd_client_search_adv(config, mpd_state, buffer, p_charbuf1, p_charbuf2, bool_buf, NULL, p_charbuf3, uint_buf1);
                free(p_charbuf1);
                free(p_charbuf2);
                free(p_charbuf3);
            }
            break;
        case MPD_API_QUEUE_SHUFFLE:
            if (mpd_run_shuffle(mpd_state->conn))
                len = snprintf(buffer, MAX_SIZE, "{\"type\": \"result\", \"data\": \"ok\"}");
            else {
                len = snprintf(buffer, MAX_SIZE, "{\"type\": \"error\", \"data\": \"Shuffling queue failed.\"}");
                printf("MPD_API_QUEUE_SHUFFLE: Error mpd_run_shuffle()\n");
            }
            break;
        case MPD_API_PLAYLIST_RM:
            je = json_scanf(request->data, request->length, "{data: {uri:%Q}}", &p_charbuf1);
            if (je == 1) {
                //remove smart playlist
                char pl_file[400];
                if (validate_string(p_charbuf1) == false) {
                    len = snprintf(buffer, MAX_SIZE, "{\"type\": \"error\", \"data\": \"Invalid filename.\"}");
                    break;
                }
                snprintf(pl_file, 400, "%s/smartpls/%s", config->varlibdir, p_charbuf1);
                if (access(pl_file, F_OK ) != -1 ) {
                    if (unlink(pl_file) == -1) {
                        len = snprintf(buffer, MAX_SIZE, "{\"type\": \"error\", \"data\": \"Deleting smart playlist failed.\"}");
                        printf("MPD_API_PLAYLIST_RM: Error unlinking smart playlist file()\n");
                        free(p_charbuf1);
                        break;
                    }
                }
                //remove mpd playlist
                if (mpd_run_rm(mpd_state->conn, p_charbuf1))
                    len = snprintf(buffer, MAX_SIZE, "{\"type\": \"result\", \"data\": \"ok\"}");
                else {
                    len = snprintf(buffer, MAX_SIZE, "{\"type\": \"error\", \"data\": \"Deleting playlist failed.\"}");
                    printf("MPD_API_QUEUE_SHUFFLE: Error mpd_run_rm()\n");
                }
                free(p_charbuf1);
            }
            break;
        case MPD_API_SETTINGS_GET:
            len = mpd_client_put_settings(mpd_state, buffer);
            break;
        case MPD_API_DATABASE_STATS:
            len = mpd_client_put_stats(mpd_state, buffer);
            break;
        default:
            len = snprintf(buffer, MAX_SIZE, "{\"type\": \"error\", \"data\": \"Unknown request\"}");
            printf("Unknown API request: %.*s\n", request->length, request->data);
    }

    if (mpd_state->conn_state == MPD_CONNECTED && mpd_connection_get_error(mpd_state->conn) != MPD_ERROR_SUCCESS) {
        printf("Error: %s\n", mpd_connection_get_error_message(mpd_state->conn));
        len = snprintf(buffer, MAX_SIZE, "{\"type\":\"error\", \"data\": \"%s\"}", 
            mpd_connection_get_error_message(mpd_state->conn));

        /* Try to recover error */
        if (!mpd_connection_clear_error(mpd_state->conn))
            mpd_state->conn_state = MPD_FAILURE;
    }

    if (len == 0) {
        len = snprintf(buffer, MAX_SIZE, "{\"type\": \"error\", \"data\": \"No response for cmd_id %u.\"}", request->cmd_id);
    }
    LOG_DEBUG() fprintf(stderr, "DEBUG: Send http response to connection %lu (first 800 chars):\n%*.*s\n", request->conn_id, 0, 800, buffer);

    t_work_result *response = (t_work_result*)malloc(sizeof(t_work_result));
    response->conn_id = request->conn_id;
    response->length = copy_string(response->data, buffer, MAX_SIZE, len);
    tiny_queue_push(web_server_queue, response);

    free(request);
}

static void mpd_client_notify(const char *message, const size_t len) {
    LOG_DEBUG() fprintf(stderr, "DEBUG: Websocket notify: %s.\n", message);
    
    t_work_result *response = (t_work_result *)malloc(sizeof(t_work_result));
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
        if (idle_name == NULL)
            break;
        if (idle_bitmask & idle_event) {
            LOG_VERBOSE() printf("MPD idle event: %s\n", idle_name);
            switch(idle_event) {
                case MPD_IDLE_DATABASE:
                    len = snprintf(buffer, MAX_SIZE, "{\"type\": \"update_database\"}");
                    mpd_client_smartpls_update_all(config, mpd_state);
                    break;
                case MPD_IDLE_STORED_PLAYLIST:
                    len = snprintf(buffer, MAX_SIZE, "{\"type\": \"update_stored_playlist\"}");
                    break;
                case MPD_IDLE_QUEUE:
                    len = snprintf(buffer, MAX_SIZE, "{\"type\": \"update_queue\"}");
                    //jukebox enabled
                    if (mpd_state->jukeboxMode != JUKEBOX_OFF)
                        mpd_client_jukebox(config, mpd_state);
                    //autoPlay enabled
                    if (mpd_state->autoPlay == true) {
                        LOG_VERBOSE() printf("AutoPlay enabled, start playing");
                        mpd_run_play(mpd_state->conn);
                    }
                    break;
                case MPD_IDLE_PLAYER:
                    len = mpd_client_put_state(mpd_state, buffer);
                    if (mpd_state->song_id != mpd_state->last_song_id) {
                        if (mpd_state->last_last_played_id != mpd_state->song_id)
                            mpd_client_last_played_list(config, mpd_state, mpd_state->song_id);
                        if (mpd_state->feat_sticker && mpd_state->last_update_sticker_song_id != mpd_state->song_id) {
                            mpd_client_count_song_id(mpd_state, mpd_state->song_id, "playCount", 1);
                            mpd_client_last_played_song_id(mpd_state, mpd_state->song_id);
                            mpd_state->last_update_sticker_song_id = mpd_state->song_id;
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
                    len = snprintf(buffer, MAX_SIZE, "{\"type\": \"update_options\"}");
                    break;
                case MPD_IDLE_UPDATE:
                    len = mpd_client_get_updatedb_state(mpd_state, buffer);
                    break;
                case MPD_IDLE_STICKER:
                    len = snprintf(buffer, MAX_SIZE, "{\"type\": \"update_sticker\"}");
                    break;
                case MPD_IDLE_SUBSCRIPTION:
                    len = snprintf(buffer, MAX_SIZE, "{\"type\": \"update_subscription\"}");
                    break;
                case MPD_IDLE_MESSAGE:
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

static void mpd_client_mpd_features(t_config *config, t_mpd_state *mpd_state) {
    struct mpd_pair *pair;
    char s[2] = ",";
    char *taglist = strdup(config->taglist);
    char *searchtaglist = strdup(config->searchtaglist);    
    char *browsetaglist = strdup(config->browsetaglist);
    char *token, *rest;    

    mpd_state->protocol = mpd_connection_get_server_version(mpd_state->conn);

    LOG_INFO() printf("MPD protocoll version: %u.%u.%u\n", mpd_state->protocol[0], mpd_state->protocol[1], mpd_state->protocol[2]);

    // Defaults
    mpd_state->feat_sticker = config->stickers;
    mpd_state->feat_playlists = false;
    mpd_state->feat_tags = false;
    mpd_state->feat_advsearch = false;
    mpd_state->feat_smartpls = config->smartpls;

    if (mpd_send_allowed_commands(mpd_state->conn)) {
        while ((pair = mpd_recv_command_pair(mpd_state->conn)) != NULL) {
            if (strcmp(pair->value, "sticker") == 0)
                mpd_state->feat_sticker = true;
            else if (strcmp(pair->value, "listplaylists") == 0)
                mpd_state->feat_playlists = true;
            mpd_return_pair(mpd_state->conn, pair);
        }
        mpd_response_finish(mpd_state->conn);
    }
    else {
        LOG_ERROR_AND_RECOVER("mpd_send_allowed_commands");
    }
    if (mpd_state->feat_sticker == true && config->stickers == false) {
        LOG_INFO() printf("MPD don't support stickers, disabling myMPD feature\n");
        mpd_state->feat_sticker = false;
    }
    if (mpd_state->feat_sticker == false && config->smartpls == true) {
        LOG_INFO() printf("Stickers are disabled, disabling smart playlists\n");
        mpd_state->feat_smartpls = false;
    }
    if (mpd_state->feat_playlists == false && config->smartpls == true) {
        LOG_INFO() printf("Playlists are disabled, disabling smart playlists\n");
        mpd_state->feat_smartpls = false;
    }
    
    LOG_INFO() printf("MPD supported tags: ");
    list_free(&mpd_state->mpd_tags);
    if (mpd_send_list_tag_types(mpd_state->conn)) {
        while ((pair = mpd_recv_tag_type_pair(mpd_state->conn)) != NULL) {
            LOG_INFO() printf("%s ", pair->value);
            list_push(&mpd_state->mpd_tags, pair->value, 1);
            mpd_return_pair(mpd_state->conn, pair);
        }
        mpd_response_finish(mpd_state->conn);
    }
    else {
        LOG_ERROR_AND_RECOVER("mpd_send_list_tag_types");
    }
    list_free(&mpd_state->mympd_tags);
    if (mpd_state->mpd_tags.length == 0) {
        LOG_INFO() printf("none\nTags are disabled\n");
        mpd_state->feat_tags = false;
    }
    else {
        mpd_state->feat_tags = true;
        LOG_INFO() printf("\nmyMPD enabled tags: ");
        #if LIBMPDCLIENT_CHECK_VERSION(2,12,0)
        enum mpd_tag_type types[64];
	unsigned n = 0;
	#endif
        token = strtok_r(taglist, s, &rest);
        while (token != NULL) {
            if (list_get_value(&mpd_state->mpd_tags, token) == 1) {
                list_push(&mpd_state->mympd_tags, token, 1);
                #if LIBMPDCLIENT_CHECK_VERSION(2,12,0)
                types[n++] = mpd_tag_name_parse(token);
                #endif
                LOG_INFO() printf("%s ", token);
            }
            token = strtok_r(NULL, s, &rest);
        }
        LOG_INFO() printf("\n");
        #if LIBMPDCLIENT_CHECK_VERSION(2,12,0)
        if (mpd_connection_cmp_server_version(mpd_state->conn, 0, 21, 0) >= 0) {
            LOG_VERBOSE() printf("Enabling mpd tag types\n");
            if (mpd_command_list_begin(mpd_state->conn, false)) {
                mpd_send_clear_tag_types(mpd_state->conn);
                mpd_send_enable_tag_types(mpd_state->conn, types, n);
                if (!mpd_command_list_end(mpd_state->conn))
                    LOG_ERROR_AND_RECOVER("mpd_command_list_end");
            }
            else
                LOG_ERROR_AND_RECOVER("mpd_command_list_begin");
            mpd_response_finish(mpd_state->conn);
        }
        #endif
        LOG_INFO() printf("myMPD enabled searchtags: ");
        token = strtok_r(searchtaglist, s, &rest);
        while (token != NULL) {
            if (list_get_value(&mpd_state->mympd_tags, token) == 1) {
                list_push(&mpd_state->mympd_searchtags, token, 1);
                LOG_INFO() printf("%s ", token);
            }
            token = strtok_r(NULL, s, &rest);
        }
        LOG_INFO() printf("\nmyMPD enabled browsetags: ");
        token = strtok_r(browsetaglist, s, &rest);
        while (token != NULL) {
            if (list_get_value(&mpd_state->mympd_tags, token) == 1) {
                list_push(&mpd_state->mympd_browsetags, token, 1);
                LOG_INFO() printf("%s ", token);
            }
            token = strtok_r(NULL, s, &rest);
        }
        LOG_INFO() printf("\n");
    }
    free(taglist);
    free(searchtaglist);
    free(browsetaglist);
    
    if (LIBMPDCLIENT_CHECK_VERSION(2, 17, 0) && mpd_connection_cmp_server_version(mpd_state->conn, 0, 21, 0) >= 0) {
        mpd_state->feat_advsearch = true;
        LOG_INFO() printf("Enabling advanced search.\n");
    } 
    else {
        LOG_INFO() printf("Disabling advanced search, depends on mpd >= 0.21.0 and libmpdclient >= 2.17.0.\n");
    }
}

static void mpd_client_idle(t_config *config, t_mpd_state *mpd_state) {
    struct pollfd fds[1];
    int pollrc;
    char buffer[MAX_SIZE];
    size_t len = 0;
    
    switch (mpd_state->conn_state) {
        case MPD_DISCONNECTED:
            /* Try to connect */
            LOG_INFO() printf("MPD Connecting to %s:%ld\n", config->mpdhost, config->mpdport);
            mpd_state->conn = mpd_connection_new(config->mpdhost, config->mpdport, mpd_state->timeout);
            if (mpd_state->conn == NULL) {
                printf("MPD connection failed.");
                len = snprintf(buffer, MAX_SIZE, "{\"type\": \"disconnected\"}");
                mpd_client_notify(buffer, len);
                mpd_state->conn_state = MPD_FAILURE;
                mpd_connection_free(mpd_state->conn);
                sleep(3);
                return;
            }

            if (mpd_connection_get_error(mpd_state->conn) != MPD_ERROR_SUCCESS) {
                printf("MPD connection: %s\n", mpd_connection_get_error_message(mpd_state->conn));
                len = snprintf(buffer, MAX_SIZE, "{\"type\": \"error\", \"data\": \"%s\"}", mpd_connection_get_error_message(mpd_state->conn));
                mpd_client_notify(buffer, len);
                mpd_state->conn_state = MPD_FAILURE;
                sleep(3);
                return;
            }

            if (config->mpdpass && !mpd_run_password(mpd_state->conn, config->mpdpass)) {
                printf("MPD connection: %s\n", mpd_connection_get_error_message(mpd_state->conn));
                len = snprintf(buffer, MAX_SIZE, "{\"type\": \"error\", \"data\": \"%s\"}", mpd_connection_get_error_message(mpd_state->conn));
                mpd_client_notify(buffer, len);
                mpd_state->conn_state = MPD_FAILURE;
                return;
            }

            LOG_INFO() printf("MPD connected.\n");
            mpd_connection_set_timeout(mpd_state->conn, mpd_state->timeout);
            mpd_state->conn_state = MPD_CONNECTED;
            mpd_client_mpd_features(config, mpd_state);
            mpd_client_smartpls_update_all(config, mpd_state);
            if (mpd_state->jukeboxMode != JUKEBOX_OFF)
                mpd_client_jukebox(config, mpd_state);
            mpd_send_idle(mpd_state->conn);
            break;

        case MPD_FAILURE:
            printf("MPD connection failed.\n");
            len = snprintf(buffer, MAX_SIZE, "{\"type\": \"disconnected\"}");
            mpd_client_notify(buffer, len);
            // fall through
        case MPD_DISCONNECT:
        case MPD_RECONNECT:
            if (mpd_state->conn != NULL) {
                mpd_connection_free(mpd_state->conn);
            }
            mpd_state->conn = NULL;
            mpd_state->conn_state = MPD_DISCONNECTED;
            break;

        case MPD_CONNECTED:
            fds[0].fd = mpd_connection_get_fd(mpd_state->conn);
            fds[0].events = POLLIN;
            pollrc = poll(fds, 1, 50);
            unsigned mpd_client_queue_length = tiny_queue_length(mpd_client_queue, 50);
            if (pollrc > 0 || mpd_client_queue_length > 0) {
                LOG_DEBUG() fprintf(stderr, "DEBUG: Leaving mpd idle mode.\n");
                mpd_send_noidle(mpd_state->conn);
                if (pollrc > 0) {
                    //Handle idle events
                    LOG_DEBUG() fprintf(stderr, "DEBUG: Checking for idle events.\n");
                    enum mpd_idle idle_bitmask = mpd_recv_idle(mpd_state->conn, false);
                    mpd_client_parse_idle(config, mpd_state, idle_bitmask);
                } 
                else {
                    mpd_response_finish(mpd_state->conn);
                }
                if (mpd_client_queue_length > 0) {
                    //Handle request
                    LOG_DEBUG() fprintf(stderr, "DEBUG: Handle request.\n");
                    struct work_request_t *request = tiny_queue_shift(mpd_client_queue, 50);
                    if (request != NULL) {
                        mpd_client_api(config, mpd_state, request);
                    }
                }
                LOG_DEBUG() fprintf(stderr, "DEBUG: Entering mpd idle mode.\n");
                mpd_send_idle(mpd_state->conn);
            }
            break;
        default:
            printf("ERROR: Invalid mpd connection state\n");
    }
}

static int mpd_client_get_updatedb_state(t_mpd_state *mpd_state, char *buffer) {
    struct mpd_status *status;
    int len, update_id;
    struct json_out out = JSON_OUT_BUF(buffer, MAX_SIZE);

    status = mpd_run_status(mpd_state->conn);
    if (!status)
        RETURN_ERROR_AND_RECOVER("mpd_run_status");
    update_id = mpd_status_get_update_id(status);
    LOG_INFO() printf("Update database ID: %d\n", update_id);
    if ( update_id > 0)
        len = json_printf(&out, "{type: update_started, data: {jobid: %d}}", update_id);
    else
        len = json_printf(&out, "{type: update_finished}");

    mpd_status_free(status);    
    
    CHECK_RETURN_LEN();
}


static bool mpd_client_get_sticker(t_mpd_state *mpd_state, const char *uri, t_sticker *sticker) {
    struct mpd_pair *pair;
    char *crap;
    sticker->playCount = 0;
    sticker->skipCount = 0;
    sticker->lastPlayed = 0;
    sticker->like = 1;
    if (uri == NULL || strncasecmp("http:", uri, 5) == 0 || strncasecmp("https:", uri, 6) == 0)
        return false;

    if (mpd_send_sticker_list(mpd_state->conn, "song", uri)) {
        while ((pair = mpd_recv_sticker(mpd_state->conn)) != NULL) {
            if (strcmp(pair->name, "playCount") == 0)
                sticker->playCount = strtol(pair->value, &crap, 10);
            else if (strcmp(pair->name, "skipCount") == 0)
                sticker->skipCount = strtol(pair->value, &crap, 10);
            else if (strcmp(pair->name, "lastPlayed") == 0)
                sticker->lastPlayed = strtol(pair->value, &crap, 10);
            else if (strcmp(pair->name, "like") == 0)
                sticker->like = strtol(pair->value, &crap, 10);
            mpd_return_sticker(mpd_state->conn, pair);
        }
    }
    else {
        LOG_ERROR_AND_RECOVER("mpd_send_sticker_list");
        return false;
    }
    return true;
}

static bool mpd_client_count_song_id(t_mpd_state *mpd_state, const int song_id, const char *name, const int value) {
    struct mpd_song *song;
    if (song_id > -1) {
        song = mpd_run_get_queue_song_id(mpd_state->conn, song_id);
        if (song) {
            if (!mpd_client_count_song_uri(mpd_state, mpd_song_get_uri(song), name, value)) {
                mpd_song_free(song);
                return false;
            }
            else {
                mpd_song_free(song);
                return true;
            }
        }
    } 
    else {
        //song_id <= 0, do nothing
    }
    return true;
}

static bool mpd_client_count_song_uri(t_mpd_state *mpd_state, const char *uri, const char *name, const int value) {
    if (uri == NULL || strncasecmp("http:", uri, 5) == 0 || strncasecmp("https:", uri, 6) == 0)
        return false;
    struct mpd_pair *pair;
    char *crap;
    int old_value = 0;
    char v[4];
    if (mpd_send_sticker_list(mpd_state->conn, "song", uri)) {
        while ((pair = mpd_recv_sticker(mpd_state->conn)) != NULL) {
            if (strcmp(pair->name, name) == 0)
                old_value = strtol(pair->value, &crap, 10);
            mpd_return_sticker(mpd_state->conn, pair);
        }
    } else {
        LOG_ERROR_AND_RECOVER("mpd_send_sticker_list");
        return false;
    }
    old_value += value;
    if (old_value > 999)
        old_value = 999;
    else if (old_value < 0)
        old_value = 0;
    snprintf(v, 4, "%d", old_value);
    LOG_VERBOSE() printf("Setting sticker: \"%s\" -> %s: %s\n", uri, name, v);
    if (!mpd_run_sticker_set(mpd_state->conn, "song", uri, name, v)) {
        LOG_ERROR_AND_RECOVER("mpd_send_sticker_set");
        return false;
    }
    return true;
}

static bool mpd_client_like_song_uri(t_mpd_state *mpd_state, const char *uri, int value) {
    if (uri == NULL || strncasecmp("http:", uri, 5) == 0 || strncasecmp("https:", uri, 6) == 0)
        return false;
    char v[2];
    if (value > 2)
        value = 2;
    else if (value < 0)
        value = 0;
    snprintf(v, 2, "%d", value);
    LOG_VERBOSE() printf("Setting sticker: \"%s\" -> like: %s\n", uri, v);
    if (!mpd_run_sticker_set(mpd_state->conn, "song", uri, "like", v)) {
        LOG_ERROR_AND_RECOVER("mpd_send_sticker_set");
        return false;
    }
    return true;        
}

static bool mpd_client_last_played_list(t_config *config, t_mpd_state *mpd_state, const int song_id) {
    struct mpd_song *song;
    char tmp_file[400];
    char cfg_file[400];
    snprintf(cfg_file, 400, "%s/state/last_played", config->varlibdir);
    snprintf(tmp_file, 400, "%s/tmp/last_played", config->varlibdir);

    if (song_id > -1) {
        song = mpd_run_get_queue_song_id(mpd_state->conn, song_id);
        if (song) {
            list_insert(&mpd_state->last_played, mpd_song_get_uri(song), time(NULL));
            mpd_state->last_last_played_id = song_id;
            mpd_song_free(song);
            if (mpd_state->last_played.length > config->last_played_count) {
                list_shift(&mpd_state->last_played, mpd_state->last_played.length -1);
            }
            FILE *fp = fopen(tmp_file, "w");
            if (fp == NULL) {
                printf("Error opening %s\n", tmp_file);
                return false;
            }
            struct node *current = mpd_state->last_played.list;
            while (current != NULL) {
                fprintf(fp, "%ld::%s\n", current->value, current->data);
                current = current->next;
            }
            fclose(fp);
            if (rename(tmp_file, cfg_file) == -1) {
                printf("Error renaming file from %s to %s\n", tmp_file, cfg_file);
                return false;
            }
        } else {
            printf("Can't get song from id %d\n", song_id);
            return false;
        }
    }
    return true;
}

static bool mpd_client_last_played_song_id(t_mpd_state *mpd_state, const int song_id) {
    struct mpd_song *song;
    
    if (song_id > -1) {
        song = mpd_run_get_queue_song_id(mpd_state->conn, song_id);
        if (song) {
            if (mpd_client_last_played_song_uri(mpd_state, mpd_song_get_uri(song)) == false) {
                mpd_song_free(song);
                return false;
            }
            else {
                mpd_song_free(song);
                return true;
            }
        }
        else {
            LOG_ERROR_AND_RECOVER("mpd_run_get_queue_song_id");
            return false;
        }
    }
    else {
        //song_id <= 0, do nothing
    }
    return true;
}

static bool mpd_client_last_played_song_uri(t_mpd_state *mpd_state, const char *uri) {
    if (uri == NULL || strncasecmp("http:", uri, 5) == 0 || strncasecmp("https:", uri, 6) == 0)
        return false;
    char v[20];
    snprintf(v, 20, "%lu", time(NULL));
    LOG_VERBOSE() printf("Setting sticker: \"%s\" -> lastPlayed: %s\n", uri, v);
    if (!mpd_run_sticker_set(mpd_state->conn, "song", uri, "lastPlayed", v)) {
        LOG_ERROR_AND_RECOVER("mpd_send_sticker_set");
        return false;
    }
    return true;
}


static char *mpd_client_get_tag(struct mpd_song const *song, const enum mpd_tag_type tag) {
    char *str;
    str = (char *)mpd_song_get_tag(song, tag, 0);
    if (str == NULL) {
        if (tag == MPD_TAG_TITLE) {
            str = basename((char *)mpd_song_get_uri(song));
        }
        else if (tag == MPD_TAG_ALBUM_ARTIST) {
            str = (char *)mpd_song_get_tag(song, MPD_TAG_ARTIST, 0);
        }
        else {
            str = strdup("-");
        }
    }
    return str;
}

static bool mpd_client_jukebox(t_config *config, t_mpd_state *mpd_state) {
    struct mpd_status *status = mpd_run_status(mpd_state->conn);
    int queue_length, addSongs, i;
    struct mpd_entity *entity;
    const struct mpd_song *song;
    struct mpd_pair *pair;
    int lineno = 1;
    int nkeep = 0;

    if (!status) {
        LOG_ERROR_AND_RECOVER("mpd_run_status");
        return false;
    }
    queue_length = mpd_status_get_queue_length(status);
    mpd_status_free(status);
    if (queue_length > mpd_state->jukeboxQueueLength)
        return true;

    if (mpd_state->jukeboxMode == JUKEBOX_ADD_SONG) 
        addSongs = mpd_state->jukeboxQueueLength - queue_length;
    else
        addSongs = 1;
    
    if (addSongs < 1)
        return true;
        
    if (mpd_state->feat_playlists == false && strcmp(mpd_state->jukeboxPlaylist, "Database") != 0) {
        LOG_INFO() printf("Jukebox: Playlists are disabled\n");
        return true;
    }
    
    //srand((unsigned int)time(NULL));

    struct list add_list;
    list_init(&add_list);
    
    if (mpd_state->jukeboxMode == JUKEBOX_ADD_SONG) {
        //add songs
        if (strcmp(mpd_state->jukeboxPlaylist, "Database") == 0) {
            if (!mpd_send_list_all(mpd_state->conn, "/")) {
                LOG_ERROR_AND_RECOVER("mpd_send_list_all");
                list_free(&add_list);
                return false;
            }
        }
        else {
            if (!mpd_send_list_playlist(mpd_state->conn, mpd_state->jukeboxPlaylist)) {
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
		        i = 0;
		        if (addSongs > 1)
		            i = randrange(addSongs);
                        if (addSongs == 1) {
                            song = mpd_entity_get_song(entity);
                            list_replace(&add_list, 0, mpd_song_get_uri(song), lineno);
                        }
                        else {
                            song = mpd_entity_get_song(entity);
                            list_replace(&add_list, i, mpd_song_get_uri(song), lineno);
                        }
                    }
                }
                lineno++;
            }
            mpd_entity_free(entity);
        }
    }
    else if (mpd_state->jukeboxMode == JUKEBOX_ADD_ALBUM) {
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
		    i = 0;
		    if (addSongs > 1)
		        i = randrange(addSongs);
                    if (addSongs == 1) {
                        list_replace(&add_list, 0, strdup(pair->value), lineno);
                    }
                    else {
                        list_replace(&add_list, i, strdup(pair->value), lineno);
                    }
                }
            }
            lineno++;
            mpd_return_pair(mpd_state->conn, pair);
        }
    }

    if (nkeep < addSongs)
        fprintf(stderr, "Warning: input didn't contain %d entries\n", addSongs);

    list_shuffle(&add_list);

    nkeep = 0;
    struct node *current = add_list.list;
    while (current != NULL) {
        if (mpd_state->jukeboxMode == JUKEBOX_ADD_SONG) {
	    LOG_INFO() printf("Jukebox adding song: %s\n", current->data);
	    if (!mpd_run_add(mpd_state->conn, current->data))
                LOG_ERROR_AND_RECOVER("mpd_run_add");
            else
                nkeep++;
        }
        else {
            LOG_INFO() printf("Jukebox adding album: %s\n", current->data);
            if (!mpd_send_command(mpd_state->conn, "searchadd", "Album", current->data, NULL)) {
                LOG_ERROR_AND_RECOVER("mpd_send_command");
                return false;
            }
            else
                nkeep++;
            mpd_response_finish(mpd_state->conn);
        }
        current = current->next;
    }
    list_free(&add_list);
    if (nkeep > 0) 
        mpd_run_play(mpd_state->conn);
    else {
        printf("Error adding song(s), trying again...\n");
        mpd_client_jukebox(config, mpd_state);
    }
    return true;
}

static int mpd_client_put_state(t_mpd_state *mpd_state, char *buffer) {
    size_t len = 0;
    struct json_out out = JSON_OUT_BUF(buffer, MAX_SIZE);

    struct mpd_status *status = mpd_run_status(mpd_state->conn);
    if (!status) {
        printf("MPD mpd_run_status: %s\n", mpd_connection_get_error_message(mpd_state->conn));
        mpd_state->conn_state = MPD_FAILURE;
        return 0;
    }
    const struct mpd_audio_format *audioformat = mpd_status_get_audio_format(status);
    const int song_id = mpd_status_get_song_id(status);
    if (mpd_state->song_id != song_id)
        mpd_state->last_song_id = mpd_state->song_id;
    
    len = json_printf(&out, "{type: update_state, data: {state: %d, volume: %d, songPos: %d, elapsedTime: %d, "
        "totalTime: %d, currentSongId: %d, kbitrate: %d, audioFormat: { sampleRate: %d, bits: %d, channels: %d}, "
        "queueLength: %d, nextSongPos: %d, nextSongId: %d, lastSongId: %d, queueVersion: %d", 
        mpd_status_get_state(status),
        mpd_status_get_volume(status), 
        mpd_status_get_song_pos(status),
        mpd_status_get_elapsed_time(status),
        mpd_status_get_total_time(status),
        song_id,
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
    
    len += json_printf(&out, "}}");
    
    mpd_state->song_id = song_id;
    mpd_state->next_song_id = mpd_status_get_next_song_id(status);
    mpd_state->queue_version = mpd_status_get_queue_version(status);
    mpd_state->queue_length = mpd_status_get_queue_length(status);
    mpd_status_free(status);

    CHECK_RETURN_LEN();
}

static int mpd_client_put_volume(t_mpd_state *mpd_state, char *buffer) {
    size_t len = 0;
    struct json_out out = JSON_OUT_BUF(buffer, MAX_SIZE);

    struct mpd_status *status = mpd_run_status(mpd_state->conn);
    if (!status) {
        printf("MPD mpd_run_status: %s\n", mpd_connection_get_error_message(mpd_state->conn));
        mpd_state->conn_state = MPD_FAILURE;
        return 0;
    }
    len = json_printf(&out, "{type: update_volume, data: {volume: %d}}",
        mpd_status_get_volume(status)
    );
    mpd_status_free(status);

    CHECK_RETURN_LEN();
}

static int mpd_client_put_settings(t_mpd_state *mpd_state, char *buffer) {
    char *replaygain = strdup("");
    size_t len;
    int nr = 0;
    struct json_out out = JSON_OUT_BUF(buffer, MAX_SIZE);
    
    struct mpd_status *status = mpd_run_status(mpd_state->conn);
    if (!status) {
        printf("MPD mpd_run_status: %s\n", mpd_connection_get_error_message(mpd_state->conn));
        mpd_state->conn_state = MPD_FAILURE;
        return 0;
    }

    if (!mpd_send_command(mpd_state->conn, "replay_gain_status", NULL)) {
        RETURN_ERROR_AND_RECOVER("replay_gain_status");
    }
    struct mpd_pair *pair;
    while ((pair = mpd_recv_pair(mpd_state->conn)) != NULL) {
        free(replaygain);
        replaygain = strdup(pair->value);
	mpd_return_pair(mpd_state->conn, pair);
    }
    
    len = json_printf(&out, "{type: settings, data: {"
        "repeat: %d, single: %d, crossfade: %d, consume: %d, random: %d, "
        "mixrampdb: %f, mixrampdelay: %f, replaygain: %Q, featPlaylists: %B, featTags: %B, featLibrary: %B, "
        "featAdvsearch: %B, featStickers: %B, featSmartpls: %B,  tags: [", 
        mpd_status_get_repeat(status),
        mpd_status_get_single(status),
        mpd_status_get_crossfade(status),
        mpd_status_get_consume(status),
        mpd_status_get_random(status),
        mpd_status_get_mixrampdb(status),
        mpd_status_get_mixrampdelay(status),
        replaygain,
        mpd_state->feat_playlists,
        mpd_state->feat_tags,
        mpd_state->feat_library,
        mpd_state->feat_advsearch,
        mpd_state->feat_sticker,
        mpd_state->feat_smartpls
    );
    mpd_status_free(status);
    free(replaygain);
    
    nr = 0;
    struct node *current = mpd_state->mympd_tags.list;
    while (current != NULL) {
        if (nr++) 
            len += json_printf(&out, ",");
        len += json_printf(&out, "%Q", current->data);
        current = current->next;
    }
    len += json_printf(&out, "], searchtags: [");
    nr = 0;
    current = mpd_state->mympd_searchtags.list;
    while (current != NULL) {
        if (nr++) 
            len += json_printf(&out, ",");
        len += json_printf(&out, "%Q", current->data);
        current = current->next;
    }
    len += json_printf(&out, "], browsetags: [");
    nr = 0;
    current = mpd_state->mympd_browsetags.list;
    while (current != NULL) {
        if (nr++) 
            len += json_printf(&out, ",");
        len += json_printf(&out, "%Q", current->data);
        current = current->next;
    }
    len += json_printf(&out, "]}}");
    
    CHECK_RETURN_LEN();
}

static int mpd_client_put_outputs(t_mpd_state *mpd_state, char *buffer) {
    struct mpd_output *output;
    size_t len;
    int nr;
    struct json_out out = JSON_OUT_BUF(buffer, MAX_SIZE);
    
    if (!mpd_send_outputs(mpd_state->conn))
        RETURN_ERROR_AND_RECOVER("outputs");

    len = json_printf(&out, "{type: outputs, data: {outputs: [");
    nr = 0;    
    while ((output = mpd_recv_output(mpd_state->conn)) != NULL) {
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

    if (!config->coverimage) {
        len = snprintf(cover, cover_len, "/assets/coverimage-notavailable.png");
    }
    else if (strncasecmp("http:", path, 5) == 0 || strncasecmp("https:", path, 6) == 0) {
        if(strlen(path) > 8) {
            if (strncasecmp("http:", path, 5) == 0)
                path += 7;
            else if (strncasecmp("https:", path, 6) == 0)
                path += 8;
            replacechar(path, '/', '_');
            replacechar(path, '.', '_');
            snprintf(cover, cover_len, "%s/pics/%s.png", DOC_ROOT, path);
            if (access(cover, F_OK ) == -1 )
                len = snprintf(cover, cover_len, "/assets/coverimage-httpstream.png");
            else
                len = snprintf(cover, cover_len, "/pics/%s.png", path);
        } else
            len = snprintf(cover, cover_len, "/assets/coverimage-httpstream.png");
    }
    else {
        if (mpd_state->feat_library) {
            dirname(path);
            snprintf(cover, cover_len, "%s/library/%s/%s", DOC_ROOT, path, config->coverimagename);
            if (access(cover, F_OK ) == -1 )
                len = snprintf(cover, cover_len, "/assets/coverimage-notavailable.png");
            else
                len = snprintf(cover, cover_len, "/library/%s/%s", path, config->coverimagename);
        } else 
            len = snprintf(cover, cover_len, "/assets/coverimage-notavailable.png");
    }
    free(orgpath);
    return len;
}

static int mpd_client_put_current_song(t_config *config, t_mpd_state *mpd_state, char *buffer) {
    size_t len = 0;
    struct json_out out = JSON_OUT_BUF(buffer, MAX_SIZE);
    char cover[500] = "";
    
    struct mpd_song *song = mpd_run_current_song(mpd_state->conn);
    if (song == NULL) {
        len = json_printf(&out, "{type: result, data: ok}");
        return len;
    }
        
    mpd_client_get_cover(config, mpd_state, mpd_song_get_uri(song), cover, 500);
    
    len = json_printf(&out, "{type: song_change, data: {pos: %d, currentSongId: %d, cover: %Q, ",
        mpd_song_get_pos(song),
        mpd_state->song_id,
        cover
    );
    if (mpd_state->feat_tags == true)
        PUT_SONG_TAGS();
    else
        PUT_MIN_SONG_TAGS();

    mpd_response_finish(mpd_state->conn);
    
    if (mpd_state->feat_sticker) {
        t_sticker *sticker = (t_sticker *) malloc(sizeof(t_sticker));
        mpd_client_get_sticker(mpd_state, mpd_song_get_uri(song), sticker);
        len += json_printf(&out, ", playCount: %d, skipCount: %d, like: %d, lastPlayed: %d",
            sticker->playCount,
            sticker->skipCount,
            sticker->like,
            sticker->lastPlayed
        );
        free(sticker);
    }
    len += json_printf(&out, "}}");
    mpd_song_free(song);

    CHECK_RETURN_LEN();
}

static int mpd_client_put_songdetails(t_config *config, t_mpd_state *mpd_state, char *buffer, const char *uri) {
    struct mpd_entity *entity;
    const struct mpd_song *song;
    size_t len = 0;
    struct json_out out = JSON_OUT_BUF(buffer, MAX_SIZE);
    char cover[500] = "";
    
    len = json_printf(&out, "{type: song_details, data: {");
    if (!mpd_send_list_all_meta(mpd_state->conn, uri))
        RETURN_ERROR_AND_RECOVER("mpd_send_list_all_meta");
    if ((entity = mpd_recv_entity(mpd_state->conn)) != NULL) {
        song = mpd_entity_get_song(entity);
        mpd_client_get_cover(config, mpd_state, uri, cover, 500);
        len += json_printf(&out, "cover: %Q, ", cover);
        if (mpd_state->feat_tags == true)
            PUT_SONG_TAGS();
        else
            PUT_MIN_SONG_TAGS();
        mpd_entity_free(entity);
    }
    mpd_response_finish(mpd_state->conn);

    if (mpd_state->feat_sticker) {
        t_sticker *sticker = (t_sticker *) malloc(sizeof(t_sticker));
        mpd_client_get_sticker(mpd_state, uri, sticker);
        len += json_printf(&out, ", playCount: %d, skipCount: %d, like: %d, lastPlayed: %d",
            sticker->playCount,
            sticker->skipCount,
            sticker->like,
            sticker->lastPlayed
        );
        free(sticker);
    }
    len += json_printf(&out, "}}");
    
    CHECK_RETURN_LEN();
}

static int mpd_client_put_last_played_songs(t_config *config, t_mpd_state *mpd_state, char *buffer, unsigned int offset) {
    const struct mpd_song *song;
    struct mpd_entity *entity;
    size_t len = 0;
    unsigned entity_count = 0;
    unsigned entities_returned = 0;
    struct json_out out = JSON_OUT_BUF(buffer, MAX_SIZE);
    
    len = json_printf(&out, "{type: last_played_songs, data: [");
    
    struct node *current = mpd_state->last_played.list;
    while (current != NULL) {
        entity_count++;
        if (entity_count > offset && entity_count <= offset + config->max_elements_per_page) {
            if (entities_returned++) 
                len += json_printf(&out, ",");
            len += json_printf(&out, "{Pos: %d, LastPlayed: %ld, ", entity_count, current->value);
            if (!mpd_send_list_all_meta(mpd_state->conn, current->data))
                RETURN_ERROR_AND_RECOVER("mpd_send_list_all_meta");
            if ((entity = mpd_recv_entity(mpd_state->conn)) != NULL) {
                song = mpd_entity_get_song(entity);
                if (mpd_state->feat_tags == true)
                    PUT_SONG_TAGS();
                else
                    PUT_MIN_SONG_TAGS();
                mpd_entity_free(entity);
                mpd_response_finish(mpd_state->conn);
            }
            len += json_printf(&out, "}");
        }
        current = current->next;
    }

    len += json_printf(&out, "], totalEntities: %d, offset: %d, returnedEntities: %d}",
        entity_count,
        offset,
        entities_returned
    );

    CHECK_RETURN_LEN();
}

static int mpd_client_put_queue(t_config *config, t_mpd_state *mpd_state, char *buffer, unsigned int offset) {
    struct mpd_entity *entity;
    int totalTime = 0;
    unsigned entity_count = 0;
    unsigned entities_returned = 0;
    size_t len;
    struct json_out out = JSON_OUT_BUF(buffer, MAX_SIZE);
    struct mpd_status *status = mpd_run_status(mpd_state->conn);
    
    if (!status)
        RETURN_ERROR_AND_RECOVER("mpd_run_status");
        
    if (!mpd_send_list_queue_range_meta(mpd_state->conn, offset, offset + config->max_elements_per_page))
        RETURN_ERROR_AND_RECOVER("mpd_send_list_queue_meta");
        
    len = json_printf(&out, "{type: queue, data: [");

    while ((entity = mpd_recv_entity(mpd_state->conn)) != NULL) {
        const struct mpd_song *song;
        unsigned int drtn;
        if (mpd_entity_get_type(entity) == MPD_ENTITY_TYPE_SONG) {
            song = mpd_entity_get_song(entity);
            drtn = mpd_song_get_duration(song);
            totalTime += drtn;
            entity_count++;
            if (entities_returned++) 
                len += json_printf(&out, ",");
            len += json_printf(&out, "{id: %d, Pos: %d, ",
                mpd_song_get_id(song),
                mpd_song_get_pos(song)
            );
            if (mpd_state->feat_tags == true)
                PUT_SONG_TAGS();
            else
                PUT_MIN_SONG_TAGS();
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

static int mpd_client_put_browse(t_config *config, t_mpd_state *mpd_state, char *buffer, const char *path, const unsigned int offset, const char *filter) {
    struct mpd_entity *entity;
    unsigned entity_count = 0;
    unsigned entities_returned = 0;
    const char *entityName;
    char smartpls_file[400];
    bool smartpls;
    size_t len = 0;
    struct json_out out = JSON_OUT_BUF(buffer, MAX_SIZE);

    if (!mpd_send_list_meta(mpd_state->conn, path))
        RETURN_ERROR_AND_RECOVER("mpd_send_list_meta");

    len = json_printf(&out, "{type: browse, data: [");

    while ((entity = mpd_recv_entity(mpd_state->conn)) != NULL) {
        entity_count++;
        if (entity_count > offset && entity_count <= offset + config->max_elements_per_page) {
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
                        if (mpd_state->feat_tags == true)
                            PUT_SONG_TAGS();
                        else
                            PUT_MIN_SONG_TAGS();

                        len += json_printf(&out, "}");
                    }
                    else {
                        entity_count--;
                    }
                    if (strncmp(entityName, "-", 1) == 0) { free((void *) entityName); }
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

    if (mpd_connection_get_error(mpd_state->conn) != MPD_ERROR_SUCCESS || !mpd_response_finish(mpd_state->conn)) {
        printf("MPD mpd_send_list_meta: %s\n", mpd_connection_get_error_message(mpd_state->conn));
        mpd_state->conn_state = MPD_FAILURE;
        return 0;
    }

    len += json_printf(&out, "], totalEntities: %d, offset: %d, returnedEntities: %d, filter: %Q}",
        entity_count,
        offset,
        entities_returned,
        filter
    );

    CHECK_RETURN_LEN();
}

static int mpd_client_put_db_tag(t_config *config, t_mpd_state *mpd_state, char *buffer, const unsigned int offset, const char *mpdtagtype, const char *mpdsearchtagtype, const char *searchstr, const char *filter) {
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
    while ((pair = mpd_recv_pair_tag(mpd_state->conn, mpd_tag_name_parse(mpdtagtype))) != NULL) {
        entity_count++;
        if (entity_count > offset && entity_count <= offset + config->max_elements_per_page) {
            if (strcmp(pair->value, "") == 0) {
                entity_count--;
            }
            else if (strncmp(filter, "-", 1) == 0 || strncasecmp(filter, pair->value, 1) == 0 ||
                    (strncmp(filter, "0", 1) == 0 && isalpha(*pair->value) == 0 )
            ) {
                if (entities_returned++) 
                    len += json_printf(&out, ", ");
                len += json_printf(&out, "{type: %Q, value: %Q}",
                    mpdtagtype,
                    pair->value    
                );
            } else {
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

static int mpd_client_put_songs_in_album(t_config *config, t_mpd_state *mpd_state, char *buffer, const char *album, const char *search, const char *tag) {
    struct mpd_song *song;
    unsigned long entity_count = 0;
    unsigned long entities_returned = 0;
    size_t len = 0;
    char cover[500] = "";
    char *albumartist = NULL;
    struct json_out out = JSON_OUT_BUF(buffer, MAX_SIZE);

    if (mpd_search_db_songs(mpd_state->conn, true) == false)
        RETURN_ERROR_AND_RECOVER("mpd_search_db_songs");
    
    if (mpd_search_add_tag_constraint(mpd_state->conn, MPD_OPERATOR_DEFAULT, mpd_tag_name_parse(tag), search) == false)
        RETURN_ERROR_AND_RECOVER("mpd_search_add_tag_constraint");

    if (mpd_search_add_tag_constraint(mpd_state->conn, MPD_OPERATOR_DEFAULT, MPD_TAG_ALBUM, album) == false)
        RETURN_ERROR_AND_RECOVER("mpd_search_add_tag_constraint");
        
    if (mpd_search_commit(mpd_state->conn) == false)
        RETURN_ERROR_AND_RECOVER("mpd_search_commit");
    else {
        len = json_printf(&out, "{type: listTitles, data: [");

        while ((song = mpd_recv_song(mpd_state->conn)) != NULL) {
            entity_count++;
            if (entity_count <= config->max_elements_per_page) {
                if (entities_returned++) 
                    len += json_printf(&out, ", ");
                else {
                    mpd_client_get_cover(config, mpd_state, mpd_song_get_uri(song), cover, 500);
                    char *value = mpd_client_get_tag(song, MPD_TAG_ALBUM_ARTIST);
                    albumartist = strdup(value);
                    if (strncmp(value, "-", 1) == 0) { free(value); }
                }
                len += json_printf(&out, "{Type: song, ");
                PUT_SONG_TAGS();
                len += json_printf(&out, "}");
            }
            mpd_song_free(song);
        }
        
        len += json_printf(&out, "], totalEntities: %d, returnedEntities: %d, Album: %Q, search: %Q, tag: %Q, cover: %Q, AlbumArtist: %Q}",
            entity_count,
            entities_returned,
            album,
            search,
            tag,
            cover,
            (albumartist != NULL ? albumartist : "-")
        );
    }
    if (albumartist != NULL)
        free(albumartist);

    CHECK_RETURN_LEN();
}

static int mpd_client_put_playlists(t_config *config, t_mpd_state *mpd_state, char *buffer, const unsigned int offset, const char *filter) {
    struct mpd_playlist *pl;
    unsigned entity_count = 0;
    unsigned entities_returned = 0;
    const char *plpath;
    size_t len = 0;
    bool smartpls;
    char smartpls_file[400];
    struct json_out out = JSON_OUT_BUF(buffer, MAX_SIZE);

    if (mpd_send_list_playlists(mpd_state->conn) == false)
        RETURN_ERROR_AND_RECOVER("mpd_send_lists_playlists");

    len = json_printf(&out, "{type: playlists, data: [");

    while ((pl = mpd_recv_playlist(mpd_state->conn)) != NULL) {
        entity_count++;
        if (entity_count > offset && entity_count <= offset + config->max_elements_per_page) {
            plpath = mpd_playlist_get_path(pl);
            if (strncmp(filter, "-", 1) == 0 || strncasecmp(filter, plpath, 1) == 0 ||
               (strncmp(filter, "0", 1) == 0 && isalpha(*plpath) == 0 )) 
            {
                if (entities_returned++)
                    len += json_printf(&out, ", ");
                snprintf(smartpls_file, 400, "%s/smartpls/%s", config->varlibdir, plpath);
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

static int mpd_client_put_playlist_list(t_config *config, t_mpd_state *mpd_state, char *buffer, const char *uri, const unsigned int offset, const char *filter) {
    struct mpd_entity *entity;
    unsigned entity_count = 0;
    unsigned entities_returned = 0;
    const char *entityName;
    char smartpls_file[400];
    bool smartpls;
    size_t len = 0;
    struct json_out out = JSON_OUT_BUF(buffer, MAX_SIZE);

    if (mpd_send_list_playlist_meta(mpd_state->conn, uri) == false)
        RETURN_ERROR_AND_RECOVER("mpd_send_list_meta");

    len = json_printf(&out, "{type: playlist_detail, data: [");

    while ((entity = mpd_recv_entity(mpd_state->conn)) != NULL) {
        const struct mpd_song *song;
        entity_count++;
        if (entity_count > offset && entity_count <= offset + config->max_elements_per_page) {
            song = mpd_entity_get_song(entity);
            entityName = mpd_client_get_tag(song, MPD_TAG_TITLE);
            if (strncmp(filter, "-", 1) == 0 || strncasecmp(filter, entityName, 1) == 0 ||
               (strncmp(filter, "0", 1) == 0 && isalpha(*entityName) == 0))
            {
                if (entities_returned++) 
                    len += json_printf(&out, ",");
                len += json_printf(&out, "{Type: song, ");
                if (mpd_state->feat_tags == true)
                    PUT_SONG_TAGS();
                else
                    PUT_MIN_SONG_TAGS();
                len += json_printf(&out, ", Pos: %d", entity_count);
                len += json_printf(&out, "}");
            }
            else {
                entity_count--;
            }
            if (strncmp(entityName, "-", 1) == 0) { free((void *) entityName); }
        }
        mpd_entity_free(entity);
    }
    
    smartpls = false;
    if (validate_string(uri) == true) {
        snprintf(smartpls_file, 400, "%s/smartpls/%s", config->varlibdir, uri);
        if (access(smartpls_file, F_OK ) != -1) {
            smartpls = true;
        }
        else {
            smartpls = false;
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

static int mpd_client_search(t_config *config, t_mpd_state *mpd_state, char *buffer, const char *searchstr, const char *filter, const char *plist, const unsigned int offset) {
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
        while ((song = mpd_recv_song(mpd_state->conn)) != NULL) {
            entity_count++;
            if (entity_count > offset && entity_count <= offset + config->max_elements_per_page) {
                if (entities_returned++) 
                    len += json_printf(&out, ", ");
                len += json_printf(&out, "{Type: song, ");
                if (mpd_state->feat_tags == true)
                    PUT_SONG_TAGS();
                else
                    PUT_MIN_SONG_TAGS();
                len += json_printf(&out, "}");
            }
            mpd_song_free(song);
        }
    }
    else
        mpd_response_finish(mpd_state->conn);

    if (strcmp(plist, "") == 0) {
        len += json_printf(&out, "], totalEntities: %d, offset: %d, returnedEntities: %d, searchstr: %Q}",
            entity_count,
            offset,
            entities_returned,
            searchstr
        );
    } 
    else
        len = json_printf(&out, "{type: result, data: ok}");

    CHECK_RETURN_LEN();
}


static int mpd_client_search_adv(t_config *config, t_mpd_state *mpd_state, char *buffer, const char *expression, const char *sort, const bool sortdesc, const char *grouptag, const char *plist, const unsigned int offset) {
    size_t len = 0;
    struct json_out out = JSON_OUT_BUF(buffer, MAX_SIZE);    
#if LIBMPDCLIENT_CHECK_VERSION(2, 17, 0)
    struct mpd_song *song;
    unsigned long entity_count = -1;
    unsigned long entities_returned = 0;
    
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
        if (mpd_search_add_window(mpd_state->conn, offset, offset + config->max_elements_per_page) == false)
            RETURN_ERROR_AND_RECOVER("mpd_search_add_window");
    }
    
    if (mpd_search_commit(mpd_state->conn) == false)
        RETURN_ERROR_AND_RECOVER("mpd_search_commit");

    if (strcmp(plist, "") == 0) {
        while ((song = mpd_recv_song(mpd_state->conn)) != NULL) {
            if (entities_returned++) 
                len += json_printf(&out, ", ");
            len += json_printf(&out, "{Type: song, ");
            if (mpd_state->feat_tags == true)
                PUT_SONG_TAGS();
            else
                PUT_MIN_SONG_TAGS();
            len += json_printf(&out, "}");
            mpd_song_free(song);
        }
    }
    else {
        mpd_response_finish(mpd_state->conn);
    }

    if (strcmp(plist, "") == 0) {
        len += json_printf(&out, "], totalEntities: %d, offset: %d, returnedEntities: %d, expression: %Q, "
            "sort: %Q, sortdesc: %B, grouptag: %Q}",
            entity_count,
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
    len = json_printf(&out, "{type: error, data: %Q}", "Advanced search is disabled.");
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
        len = json_printf(&out, "{type: error, data: %Q}", "A playlist longer than 1 song in length is required to crop.");
        printf("A playlist longer than 1 song in length is required to crop.\n");
    }
    else if (mpd_status_get_state(status) == MPD_STATE_PLAY || mpd_status_get_state(status) == MPD_STATE_PAUSE) {
        playing_song_pos++;
        if (playing_song_pos < length)
            mpd_run_delete_range(mpd_state->conn, playing_song_pos, -1);
        playing_song_pos--;
        if (playing_song_pos > 0 )
            mpd_run_delete_range(mpd_state->conn, 0, playing_song_pos--);            
        len = json_printf(&out, "{type: result, data: ok}");
    } else {
        len = json_printf(&out, "{type: error, data: %Q}", "You need to be playing to crop the playlist");
        printf("You need to be playing to crop the playlist\n");
    }
    
    mpd_status_free(status);
    
    CHECK_RETURN_LEN();
}

static int mpd_client_search_queue(t_config *config, t_mpd_state *mpd_state, char *buffer, const char *mpdtagtype, const unsigned int offset, const char *searchstr) {
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

        while ((song = mpd_recv_song(mpd_state->conn)) != NULL) {
            entity_count++;
            if (entity_count > offset && entity_count <= offset + config->max_elements_per_page) {
                if (entities_returned++)
                    len += json_printf(&out, ", ");
                len += json_printf(&out, "{type: song, id: %d, Pos: %d, ",
                    mpd_song_get_id(song),
                    mpd_song_get_pos(song)
                );
                if (mpd_state->feat_tags == true)
                    PUT_SONG_TAGS();
                else
                    PUT_MIN_SONG_TAGS();
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

static void mpd_client_disconnect(t_config *config, t_mpd_state *mpd_state) {
    mpd_state->conn_state = MPD_DISCONNECT;
    if (mpd_state->conn != NULL) {
        mpd_connection_free(mpd_state->conn);
    }
}

static int mpd_client_smartpls_put(t_config *config, char *buffer, const char *playlist) {
    char pl_file[400];
    char *smartpltype;
    char *p_charbuf1, *p_charbuf2;
    int je, int_buf1;
    size_t len = 0;
    struct json_out out = JSON_OUT_BUF(buffer, MAX_SIZE);
    
    if (validate_string(playlist) == false) {
        len = json_printf(&out, "{type: error, data: %Q}}", "Can't read smart playlist file");
        return len;
    }
    snprintf(pl_file, 400, "%s/smartpls/%s", config->varlibdir, playlist);
    char *content = json_fread(pl_file);
    if (content == NULL) {
        len = json_printf(&out, "{type: error, data: %Q}}", "Can't read smart playlist file");
        printf("Can't read smart playlist file: %s\n", pl_file);
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
                free(p_charbuf1);
            } else {
                len = json_printf(&out, "{type: error, data: %Q]", "Can't parse smart playlist file");
                printf("Can't parse smart playlist file: %s\n", pl_file);
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
                len = json_printf(&out, "{type: error, data: %Q]", "Can't parse smart playlist file");
                printf("Can't parse smart playlist file: %s\n", pl_file);
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
                free(p_charbuf1);
                free(p_charbuf2);
            } else {
                len = json_printf(&out, "{type: error, data: %Q]", "Can't parse smart playlist file");
                printf("Can't parse smart playlist file: %s\n", pl_file);
            }
        } else {
            len = json_printf(&out, "{type: error, data: %Q}}", "Unknown smart playlist type");
            printf("Unknown smart playlist type: %s\n", pl_file);
        }
        free(smartpltype);        
    } else {
        len = json_printf(&out, "{type: error, data: %Q}}", "Unknown smart playlist type");
        printf("Unknown smart playlist type: %s\n", pl_file);
    }
    free(content);
    return len;
}

static bool mpd_client_smartpls_save(t_config *config, t_mpd_state *mpd_state, const char *smartpltype, const char *playlist, const char *tag, const char *searchstr, const int maxentries, const int timerange) {
    char tmp_file[400];
    char pl_file[400];
    if (validate_string(playlist) == false) {
        return false;
    }
    snprintf(tmp_file, 400, "%s/tmp/%s", config->varlibdir, playlist);
    snprintf(pl_file, 400, "%s/smartpls/%s", config->varlibdir, playlist);
    if (strcmp(smartpltype, "sticker") == 0) {
        if (json_fprintf(tmp_file, "{type: %Q, sticker: %Q, maxentries: %d}", smartpltype, tag, maxentries) == -1) {
            printf("Error creating file %s\n", tmp_file);
            return false;
        }
        else if (rename(tmp_file, pl_file) == -1) {
            printf("Error renaming file from %s to %s\n", tmp_file, pl_file);
            return false;
        }
        else if (mpd_client_smartpls_update_sticker(mpd_state, playlist, tag, maxentries) == false) {
            printf("Update of smart playlist %s failed.\n", playlist);
            return false;
        }
    }
    else if (strcmp(smartpltype, "newest") == 0) {
        if (json_fprintf(tmp_file, "{type: %Q, timerange: %d}", smartpltype, timerange) == -1) {
            printf("Error creating file %s\n", tmp_file);
            return false;
        }
        else if (rename(tmp_file, pl_file) == -1) {
            printf("Error renaming file from %s to %s\n", tmp_file, pl_file);
            return false;
        }
        else if (mpd_client_smartpls_update_newest(config, mpd_state, playlist, timerange) == false) {
            printf("Update of smart playlist %s failed.\n", playlist);
            return false;
        }
    }
    else if (strcmp(smartpltype, "search") == 0) {
        if (json_fprintf(tmp_file, "{type: %Q, tag: %Q, searchstr: %Q}", smartpltype, tag, searchstr) == -1) {
            printf("Error creating file %s\n", tmp_file);
            return false;
        }
        else if (rename(tmp_file, pl_file) == -1) {
            printf("Error renaming file from %s to %s\n", tmp_file, pl_file);
            return false;
        }
        else if (mpd_client_smartpls_update_search(config, mpd_state, playlist, tag, searchstr) == false) {
            printf("Update of smart playlist %s failed.\n", playlist);
            return false;
        }
    }
    return true;
}

static bool mpd_client_smartpls_update_all(t_config *config, t_mpd_state *mpd_state) {
    DIR *dir;
    struct dirent *ent;
    char *smartpltype;
    char filename[400];
    char dirname[400];
    int je;
    char *p_charbuf1, *p_charbuf2;
    int int_buf1;
    
    if (mpd_state->feat_smartpls == false) {
        return true;
    }
    
    snprintf(dirname, 400, "%s/smartpls", config->varlibdir);
    if ((dir = opendir (dirname)) != NULL) {
        while ((ent = readdir(dir)) != NULL) {
            if (strncmp(ent->d_name, ".", 1) == 0)
                continue;
            snprintf(filename, 400, "%s/smartpls/%s", config->varlibdir, ent->d_name);
            char *content = json_fread(filename);
            if (content == NULL) {
                printf("Cant read smart playlist file %s\n", filename);
                continue;
            }
            je = json_scanf(content, strlen(content), "{type: %Q }", &smartpltype);
            if (je != 1)
                continue;
            if (strcmp(smartpltype, "sticker") == 0) {
                je = json_scanf(content, strlen(content), "{sticker: %Q, maxentries: %d}", &p_charbuf1, &int_buf1);
                if (je == 2) {
                    if (mpd_client_smartpls_update_sticker(mpd_state, ent->d_name, p_charbuf1, int_buf1) == false) {
                        printf("Update of smart playlist %s failed.\n", ent->d_name);
                    }
                    free(p_charbuf1);
                }
                else {
                    printf("Can't parse smart playlist file %s\n", filename);
                }
            }
            else if (strcmp(smartpltype, "newest") == 0) {
                je = json_scanf(content, strlen(content), "{timerange: %d}", &int_buf1);
                if (je == 1) {
                    if (mpd_client_smartpls_update_newest(config, mpd_state, ent->d_name, int_buf1) == false) {
                        printf("Update of smart playlist %s failed.\n", ent->d_name);
                    }
                }
                else {
                    printf("Can't parse smart playlist file %s\n", filename);
                }
            }
            else if (strcmp(smartpltype, "search") == 0) {
                je = json_scanf(content, strlen(content), "{tag: %Q, searchstr: %Q}", &p_charbuf1, &p_charbuf2);
                if (je == 2) {
                    if (mpd_client_smartpls_update_search(config, mpd_state, ent->d_name, p_charbuf1, p_charbuf2) == false) {
                        printf("Update of smart playlist %s failed.\n", ent->d_name);
                    }
                    free(p_charbuf1);
                    free(p_charbuf2);
                }
                else {
                    printf("Can't parse smart playlist file %s\n", filename);
                }
            }
            free(smartpltype);
            free(content);
        }
        closedir (dir);
    } else {
        printf("Can't open smart playlist directory %s\n", dirname);
        return false;
    }
    return true;
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

static bool mpd_client_smartpls_update_search(t_config *config, t_mpd_state *mpd_state, const char *playlist, const char *tag, const char *searchstr) {
    char buffer[MAX_SIZE];
    mpd_client_smartpls_clear(mpd_state, playlist);
    if (mpd_state->feat_advsearch == true && strcmp(tag, "expression") == 0) {
        mpd_client_search_adv(config, mpd_state, buffer, searchstr, NULL, true, NULL, playlist, 0);
    }
    else {
        mpd_client_search(config, mpd_state, buffer, searchstr, tag, playlist, 0);
    }
    LOG_INFO() printf("Updated smart playlist %s\n", playlist);
    return true;
}

static bool mpd_client_smartpls_update_sticker(t_mpd_state *mpd_state, const char *playlist, const char *sticker, const int maxentries) {
    struct mpd_pair *pair;
    char *uri = NULL;
    const char *p_value;
    char *crap;
    long value;
    long value_max = 0;
    long i = 0;
    size_t j;

    if (mpd_send_sticker_find(mpd_state->conn, "song", "", sticker) == false) {
        LOG_ERROR_AND_RECOVER("mpd_send_sticker_find");
        return false;    
    }

    struct list add_list;
    list_init(&add_list);

    while ((pair = mpd_recv_pair(mpd_state->conn)) != NULL) {
        if (strcmp(pair->name, "file") == 0) {
            if (uri != NULL) {
                free(uri);
                uri = NULL;
            }
            uri = strdup(pair->value);
        } 
        else if (strcmp(pair->name, "sticker") == 0) {
            p_value = mpd_parse_sticker(pair->value, &j);
            if (p_value != NULL) {
                value = strtol(p_value, &crap, 10);
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
    if (uri != NULL) {
        free(uri);
        uri = NULL;
    }

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
    LOG_INFO() printf("Updated smart playlist %s with %ld songs, minValue: %ld\n", playlist, i, value_max);
    return true;
}

static bool mpd_client_smartpls_update_newest(t_config *config, t_mpd_state *mpd_state, const char *playlist, const int timerange) {
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
            mpd_client_search_adv(config, mpd_state, buffer, searchstr, NULL, true, NULL, playlist, 0);
        }
        else {
            snprintf(searchstr, 20, "%d", value_max);
            mpd_client_search(config, mpd_state, buffer, searchstr, "modified-since", playlist, 0);
        }
        LOG_INFO() printf("Updated smart playlist %s\n", playlist);
    }
    return true;
}

static int mpd_client_read_last_played(t_config *config, t_mpd_state *mpd_state) {
    char cfgfile[400];
    char *line = NULL;
    char *data;
    char *crap;
    size_t n = 0;
    ssize_t read;
    long value;
    
    snprintf(cfgfile, 400, "%s/state/last_played", config->varlibdir);
    FILE *fp = fopen(cfgfile, "r");
    if (fp == NULL) {
        printf("Error opening %s\n", cfgfile);
        return 0;
    }
    while ((read = getline(&line, &n, fp)) > 0) {
        value = strtol(line, &data, 10);
        if (strlen(data) > 2) {
            data = data + 2;
            strtok_r(data, "\n", &crap);
            list_push(&mpd_state->last_played, data, value);
        }
        else {
            printf("Error reading last_played line.\n");
        }
    }
    fclose(fp);
    free(line);
    return mpd_state->last_played.length;;
}
