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
#include <dirent.h>
#include <stdbool.h>
#include <pthread.h>
#include <signal.h>
#include <assert.h>
#include <inttypes.h>

#include "api.h"
#include "utility.h"
#include "log.h"
#include "list.h"
#include "tiny_queue.h"
#include "config_defs.h"
#include "global.h"
#include "mympd_api.h"
#include "mpd_client.h"
#include "../dist/src/frozen/frozen.h"

//private definitions
typedef struct t_mympd_state {
    char *mpd_host;
    int mpd_port;
    char *mpd_pass;
    bool stickers;
    char *taglist;
    char *searchtaglist;
    char *browsetaglist;
    bool smartpls;
    int max_elements_per_page;
    int last_played_count;
    bool love;
    char *love_channel;
    char *love_message;
    bool notification_web;
    bool notification_page;
    bool auto_play;
    enum jukebox_modes jukebox_mode;
    char *jukebox_playlist;
    int jukebox_queue_length;
    char *cols_queue_current;
    char *cols_search;
    char *cols_browse_database;
    char *cols_browse_playlists_detail;
    char *cols_browse_filesystem;
    char *cols_playback;
    char *cols_queue_last_played;
    bool localplayer;
    bool localplayer_autoplay;
    int stream_port;
    char *stream_url;
    bool bg_cover;
    char *bg_color;
    char *bg_css_filter;
    bool coverimage;
    char *coverimage_name;
    int coverimage_size;
    char *locale;
    char *music_directory;
} t_mympd_state;

static void mympd_api(t_config *config, t_mympd_state *mympd_state, t_work_request *request);
static void mympd_api_push_to_mpd_client(t_mympd_state *mympd_state);
static int mympd_api_syscmd(t_config *config, char *buffer, const char *cmd);
static void mympd_api_read_statefiles(t_config *config, t_mympd_state *mympd_state);
static char *state_file_rw_string(t_config *config, const char *name, const char *def_value, bool warn);
static bool state_file_rw_bool(t_config *config, const char *name, const bool def_value, bool warn);
static int state_file_rw_int(t_config *config, const char *name, const int def_value, bool warn);
static bool state_file_write(t_config *config, const char *name, const char *value);
static int mympd_api_put_settings(t_config *config, t_mympd_state *mympd_state, char *buffer);
static void mympd_api_settings_reset(t_config *config, t_mympd_state *mympd_state);
static bool mympd_api_bookmark_update(t_config *config, const int id, const char *name, const char *uri, const char *type);
static int mympd_api_bookmark_list(t_config *config, char *buffer, unsigned int offset);

//public functions
void *mympd_api_loop(void *arg_config) {
    t_config *config = (t_config *) arg_config;
    
    //read myMPD states under config.varlibdir
    t_mympd_state *mympd_state = (t_mympd_state *)malloc(sizeof(t_mympd_state));
    
    mympd_api_read_statefiles(config, mympd_state);

    //push settings to mpd_client queue
    mympd_api_push_to_mpd_client(mympd_state);

    while (s_signal_received == 0) {
        struct t_work_request *request = tiny_queue_shift(mympd_api_queue, 0);
        if (request != NULL) {
            mympd_api(config, mympd_state, request);
        }
    }

    FREE_PTR(mympd_state->mpd_host);
    FREE_PTR(mympd_state->mpd_pass);
    FREE_PTR(mympd_state->taglist);
    FREE_PTR(mympd_state->searchtaglist);
    FREE_PTR(mympd_state->browsetaglist);
    FREE_PTR(mympd_state->love_channel);
    FREE_PTR(mympd_state->love_message);
    FREE_PTR(mympd_state->jukebox_playlist);
    FREE_PTR(mympd_state->cols_queue_current);
    FREE_PTR(mympd_state->cols_search);
    FREE_PTR(mympd_state->cols_browse_database);
    FREE_PTR(mympd_state->cols_browse_playlists_detail);
    FREE_PTR(mympd_state->cols_browse_filesystem);
    FREE_PTR(mympd_state->cols_playback);
    FREE_PTR(mympd_state->cols_queue_last_played);
    FREE_PTR(mympd_state->stream_url);
    FREE_PTR(mympd_state->bg_color);
    FREE_PTR(mympd_state->bg_css_filter);
    FREE_PTR(mympd_state->coverimage_name);
    FREE_PTR(mympd_state->locale);
    FREE_PTR(mympd_state->music_directory);
    free(mympd_state);
    mympd_state = NULL;
    return NULL;
}

static void mympd_api_push_to_mpd_client(t_mympd_state *mympd_state) {
    t_work_request *mpd_client_request = (t_work_request *)malloc(sizeof(t_work_request));
    assert(mpd_client_request);
    struct json_out out = JSON_OUT_BUF(mpd_client_request->data, 1000);
    mpd_client_request->conn_id = -1;
    mpd_client_request->cmd_id = MYMPD_API_SETTINGS_SET;
    mpd_client_request->length = json_printf(&out, "{cmd: MYMPD_API_SETTINGS_SET,data:{"
        "jukeboxMode: %d,"
        "jukeboxPlaylist: %Q,"
        "jukeboxQueueLength: %d,"
        "autoPlay: %B,"
        "coverimage: %B,"
        "coverimageName: %Q,"
        "love: %B,"
        "loveChannel: %Q,"
        "loveMessage: %Q,"
        "taglist: %Q,"
        "searchtaglist: %Q,"
        "browsetaglist: %Q,"
        "stickers: %B,"
        "smartpls: %B,"
        "mpdHost: %Q,"
        "mpdPort: %d,"
        "mpdPass: %Q,"
        "lastPlayedCount: %d,"
        "maxElementsPerPage: %d,"
        "musicDirectory: %Q"
        "}}",
        mympd_state->jukebox_mode,
        mympd_state->jukebox_playlist,
        mympd_state->jukebox_queue_length,
        mympd_state->auto_play,
        mympd_state->coverimage,
        mympd_state->coverimage_name,
        mympd_state->love,
        mympd_state->love_channel,
        mympd_state->love_message,
        mympd_state->taglist,
        mympd_state->searchtaglist,
        mympd_state->browsetaglist,
        mympd_state->stickers,
        mympd_state->smartpls,
        mympd_state->mpd_host,
        mympd_state->mpd_port,
        mympd_state->mpd_pass,
        mympd_state->last_played_count,
        mympd_state->max_elements_per_page,
        mympd_state->music_directory
    );
    tiny_queue_push(mpd_client_queue, mpd_client_request);
}

void mympd_api_settings_delete(t_config *config) {
    const char* state_files[]={"auto_play", "bg_color", "bg_cover", "bg_css_filter", "browsetaglist", "cols_browse_database",
        "cols_browse_filesystem", "cols_browse_playlists_detail", "cols_playback", "cols_queue_current", "cols_queue_last_played",
        "cols_search", "coverimage", "coverimage_name", "coverimage_size", "jukebox_mode", "jukebox_playlist", "jukebox_queue_length",
        "last_played", "last_played_count", "locale", "localplayer", "localplayer_autoplay", "love", "love_channel", "love_message",
        "max_elements_per_page",  "mpd_host", "mpd_pass", "mpd_port", "notification_page", "notification_web", "searchtaglist",
        "smartpls", "stickers", "stream_port", "stream_url", "taglist", "music_directory", 0};
    const char** ptr = state_files;
    while (*ptr != 0) {
        size_t filename_len = strlen(*ptr) + config->varlibdir_len + 8;
        char filename[filename_len];
        snprintf(filename, filename_len, "%s/state/%s", config->varlibdir, *ptr);
        unlink(filename);
        ++ptr;
    }
}

//private functions
static void mympd_api(t_config *config, t_mympd_state *mympd_state, t_work_request *request) {
    int je;
    char *p_charbuf1 = NULL;
    char *p_charbuf2 = NULL;
    char *p_charbuf3 = NULL;
    char p_char[7];
    unsigned int uint_buf1;
    int int_buf1;
    bool bool_buf;
    LOG_VERBOSE("MYMPD API request: %.*s", request->length, request->data);
    
    //create response struct
    t_work_result *response = (t_work_result *)malloc(sizeof(t_work_result));
    assert(response);
    response->conn_id = request->conn_id;
    response->length = 0;
    
    if (request->cmd_id == MYMPD_API_SYSCMD) {
        if (config->syscmds == true) {
            je = json_scanf(request->data, request->length, "{data: {cmd: %Q}}", &p_charbuf1);
            if (je == 1) {
                response->length = mympd_api_syscmd(config, response->data, p_charbuf1);
                FREE_PTR(p_charbuf1);
            }
        } 
        else {
            response->length = snprintf(response->data, MAX_SIZE, "{\"type\": \"error\", \"data\": \"System commands are disabled\"}");
        }
    }
    else if (request->cmd_id == MYMPD_API_COLS_SAVE) {
        je = json_scanf(request->data, request->length, "{data: {table: %Q}}", &p_charbuf1);
        if (je == 1) {
            char column_list[request->length + 1];
            snprintf(column_list, request->length + 1, "%.*s", request->length, request->data);
            char *cols = strchr(column_list, '[');
            int col_len = strlen(cols); 
            if (col_len > 1)
                cols[col_len - 2]  = '\0';
            if (strcmp(p_charbuf1, "colsQueueCurrent") == 0) {
                FREE_PTR(mympd_state->cols_queue_current);
                mympd_state->cols_queue_current = strdup(cols);
            }
            else if (strcmp(p_charbuf1, "colsSearch") == 0) {
                FREE_PTR(mympd_state->cols_search);
                mympd_state->cols_search = strdup(cols);
            }
            else if (strcmp(p_charbuf1, "colsBrowseDatabase") == 0) {
                FREE_PTR(mympd_state->cols_browse_database);
                mympd_state->cols_browse_database = strdup(cols);
            }
            else if (strcmp(p_charbuf1, "colsBrowsePlaylistsDetail") == 0) {
                FREE_PTR(mympd_state->cols_browse_playlists_detail);
                mympd_state->cols_browse_playlists_detail = strdup(cols);
            }
            else if (strcmp(p_charbuf1, "colsBrowseFilesystem") == 0) {
                FREE_PTR(mympd_state->cols_browse_filesystem);
                mympd_state->cols_browse_filesystem = strdup(cols);
            }
            else if (strcmp(p_charbuf1, "colsPlayback") == 0) {
                FREE_PTR(mympd_state->cols_playback);
                mympd_state->cols_playback = strdup(cols);
            }
            else if (strcmp(p_charbuf1, "colsQueueLastPlayed") == 0) {
                FREE_PTR(mympd_state->cols_queue_last_played);
                mympd_state->cols_queue_last_played = strdup(cols);
            }
            else {
                response->length = snprintf(response->data, MAX_SIZE, "{\"type\": \"error\", \"data\": \"Unknown table %%{table}\", \"values\": {\"table\": \"%s\"}}", p_charbuf1);
                LOG_ERROR("MYMPD_API_COLS_SAVE: Unknown table %s", p_charbuf1);
            }
            if (response->length == 0) {
                if (state_file_write(config, p_charbuf1, cols))
                    response->length = snprintf(response->data, MAX_SIZE, "{\"type\": \"result\", \"data\": \"ok\"}");
            }
            FREE_PTR(p_charbuf1);
        }
    }
    else if (request->cmd_id == MYMPD_API_SETTINGS_RESET) {
        mympd_api_settings_reset(config, mympd_state);
        response->length = snprintf(response->data, MAX_SIZE, "{\"type\": \"result\", \"data\": \"ok\"}");
    }
    else if (request->cmd_id == MYMPD_API_SETTINGS_SET) {
        je = json_scanf(request->data, request->length, "{data: {notificationWeb: %B}}", &bool_buf);
        if (je == 1) {
            mympd_state->notification_web = bool_buf;
            if (!state_file_write(config, "notification_web", (mympd_state->notification_web == true ? "true" : "false")))
                response->length = snprintf(response->data, MAX_SIZE, "{\"type\": \"error\", \"data\": \"Failed to set myMPD state %%{state}\", \"values\": {\"state\": \"notification_web\"}}");
        }    
        je = json_scanf(request->data, request->length, "{data: {notificationPage: %B}}", &bool_buf);
        if (je == 1) {
            mympd_state->notification_page = bool_buf;
            if (!state_file_write(config, "notification_page", (mympd_state->notification_page == true ? "true" : "false")))
                response->length = snprintf(response->data, MAX_SIZE, "{\"type\": \"error\", \"data\": \"Failed to set myMPD state %%{state}\", \"values\": {\"state\": \"notification_page\"}}");
        }
        je = json_scanf(request->data, request->length, "{data: {autoPlay: %B}}", &bool_buf);
        if (je == 1) {
            mympd_state->auto_play = bool_buf;
            if (!state_file_write(config, "auto_play", (mympd_state->auto_play == true ? "true" : "false")))
                response->length = snprintf(response->data, MAX_SIZE, "{\"type\": \"error\", \"data\": \"Failed to set myMPD state %%{state}\", \"values\": {\"state\": \"auto_play\"}}");
        }
        je = json_scanf(request->data, request->length, "{data: {localplayerAutoplay: %B}}", &bool_buf);
        if (je == 1) {
            mympd_state->localplayer_autoplay = bool_buf;
            if (!state_file_write(config, "localplayer_autoplay", (mympd_state->localplayer_autoplay == true ? "true" : "false")))
                response->length = snprintf(response->data, MAX_SIZE, "{\"type\": \"error\", \"data\": \"Failed to set myMPD state %%{state}\", \"values\": {\"state\": \"localplayer_autoplay\"}}");
        }
        je = json_scanf(request->data, request->length, "{data: {coverimage: %B}}", &bool_buf);
        if (je == 1) {
            mympd_state->coverimage = bool_buf;
            if (!state_file_write(config, "coverimage", (mympd_state->coverimage == true ? "true" : "false")))
                response->length = snprintf(response->data, MAX_SIZE, "{\"type\": \"error\", \"data\": \"Failed to set myMPD state %%{state}\", \"values\": {\"state\": \"coverimage\"}}");
        }
        je = json_scanf(request->data, request->length, "{data: {coverimageName: %Q}}", &p_charbuf1);
        if (je == 1) {
            if (validate_string(p_charbuf1) && strlen(p_charbuf1) > 0) {
                REASSIGN_PTR(mympd_state->coverimage_name, p_charbuf1);
                if (!state_file_write(config, "coverimage_name", mympd_state->coverimage_name))
                    response->length = snprintf(response->data, MAX_SIZE, "{\"type\": \"error\", \"data\": \"Failed to set myMPD state %%{state}\", \"values\": {\"state\": \"coverimage_name\"}}");
            }
            else {
                FREE_PTR(p_charbuf1);
                response->length = snprintf(response->data, MAX_SIZE, "{\"type\": \"error\", \"data\": \"Invalid filename for coverimage_name\"}");
            }
        }
        
        je = json_scanf(request->data, request->length, "{data: {coverimageSize: %d}}", &int_buf1);
        if (je == 1) {
            mympd_state->coverimage_size = int_buf1;
            snprintf(p_char, 7, "%d", mympd_state->coverimage_size);
            if (!state_file_write(config, "coverimage_size", p_char))
                response->length = snprintf(response->data, MAX_SIZE, "{\"type\": \"error\", \"data\": \"Failed to set myMPD state %%{state}\", \"values\": {\"state\": \"coverimage_size\"}}");
        }
        je = json_scanf(request->data, request->length, "{data: {featLocalplayer: %B}}", &bool_buf);
        if (je == 1) {
            mympd_state->localplayer = bool_buf;
            if (!state_file_write(config, "localplayer", (mympd_state->localplayer == true ? "true" : "false")))
                response->length = snprintf(response->data, MAX_SIZE, "{\"type\": \"error\", \"data\": \"Failed to set myMPD state %%{state}\", \"values\": {\"state\": \"localplayer\"}}");
        }
        je = json_scanf(request->data, request->length, "{data: {streamPort: %d}}", &int_buf1);
        if (je == 1) {
            mympd_state->stream_port = int_buf1;
            snprintf(p_char, 7, "%d", mympd_state->stream_port);
            if (!state_file_write(config, "stream_port", p_char))
                response->length = snprintf(response->data, MAX_SIZE, "{\"type\": \"error\", \"data\": \"Failed to set myMPD state %%{state}\", \"values\": {\"state\": \"stream_port\"}}");
        }
        je = json_scanf(request->data, request->length, "{data: {streamUrl: %Q}}", &p_charbuf1);
        if (je == 1) {
            REASSIGN_PTR(mympd_state->stream_url, p_charbuf1);
            if (!state_file_write(config, "stream_url", mympd_state->stream_url))
                response->length = snprintf(response->data, MAX_SIZE, "{\"type\": \"error\", \"data\": \"Failed to set myMPD state %%{state}\", \"values\": {\"state\": \"stream_url\"}}");
        }
        je = json_scanf(request->data, request->length, "{data: {locale: %Q}}", &p_charbuf1);
        if (je == 1) {
            REASSIGN_PTR(mympd_state->locale, p_charbuf1);
            if (!state_file_write(config, "locale", mympd_state->locale))
                response->length = snprintf(response->data, MAX_SIZE, "{\"type\": \"error\", \"data\": \"Failed to set myMPD state %%{state}\", \"values\": {\"state\": \"locale\"}}");
        }
        je = json_scanf(request->data, request->length, "{data: {bgCover: %B}}", &bool_buf);
        if (je == 1) {
            mympd_state->bg_cover = bool_buf;
            if (!state_file_write(config, "bg_cover", (mympd_state->bg_cover == true ? "true" : "false")))
                response->length = snprintf(response->data, MAX_SIZE, "{\"type\": \"error\", \"data\": \"Failed to set myMPD state %%{state}\", \"values\": {\"state\": \"bg_cover\"}}");
        }
        je = json_scanf(request->data, request->length, "{data: {bgColor: %Q}}", &p_charbuf1);
        if (je == 1) {
            REASSIGN_PTR(mympd_state->bg_color, p_charbuf1);
            if (!state_file_write(config, "bg_color", mympd_state->bg_color))
                response->length = snprintf(response->data, MAX_SIZE, "{\"type\": \"error\", \"data\": \"Failed to set myMPD state %%{state}\", \"values\": {\"state\": \"bg_color\"}}");
        }
        je = json_scanf(request->data, request->length, "{data: {bgCssFilter: %Q}}", &p_charbuf1);
        if (je == 1) {
            REASSIGN_PTR(mympd_state->bg_css_filter, p_charbuf1);
            if (!state_file_write(config, "bg_css_filter", mympd_state->bg_css_filter))
                response->length = snprintf(response->data, MAX_SIZE, "{\"type\": \"error\", \"data\": \"Failed to set myMPD state %%{state}\", \"values\": {\"state\": \"bg_css_filter\"}}");
        }
        je = json_scanf(request->data, request->length, "{data: {jukeboxMode: %u}}", &uint_buf1);
        if (je == 1) {
            mympd_state->jukebox_mode = uint_buf1;
            if (mympd_state->jukebox_mode > 2) {
                response->length = snprintf(response->data, MAX_SIZE, "{\"type\": \"error\", \"data\": \"Invalid jukebox mode\"}");
                LOG_ERROR("Invalid jukeboxMode");
                mympd_state->jukebox_mode = JUKEBOX_OFF;
            }
            snprintf(p_char, 7, "%d", mympd_state->jukebox_mode);
            if (!state_file_write(config, "jukebox_mode", p_char))
                response->length = snprintf(response->data, MAX_SIZE, "{\"type\": \"error\", \"data\": \"Failed to set myMPD state %%{state}\", \"values\": {\"state\": \"jukebox_mode\"}}");
        }
        je = json_scanf(request->data, request->length, "{data: {jukeboxPlaylist: %Q}}", &p_charbuf1);
        if (je == 1) {
            REASSIGN_PTR(mympd_state->jukebox_playlist, p_charbuf1);
            if (!state_file_write(config, "jukebox_playlist", mympd_state->jukebox_playlist))
                response->length = snprintf(response->data, MAX_SIZE, "{\"type\": \"error\", \"data\": \"Failed to set myMPD state %%{state}\", \"values\": {\"state\": \"jukebox_playlist\"}}");
        }
        je = json_scanf(request->data, request->length, "{data: {jukeboxQueueLength: %d}}", &int_buf1);
        if (je == 1) {
            mympd_state->jukebox_queue_length = int_buf1;
            if (mympd_state->jukebox_queue_length > 999) {
                LOG_WARN("jukebox_queue_length to big, setting it to maximum value of 999");
                response->length = snprintf(response->data, MAX_SIZE, "{\"type\": \"error\", \"data\": \"jukebox_queue_length to big, setting it to maximum value of 999\"}");
                mympd_state->jukebox_queue_length = 999;
            }
            snprintf(p_char, 7, "%d", mympd_state->jukebox_queue_length);
            if (!state_file_write(config, "jukebox_queue_length", p_char))
                response->length = snprintf(response->data, MAX_SIZE, "{\"type\": \"error\", \"data\": \"Failed to set myMPD state %%{state}\", \"values\": {\"state\": \"jukebox_queue_length\"}}");
        }
        je = json_scanf(request->data, request->length, "{data: {stickers: %B}}", &bool_buf);
        if (je == 1) {
            mympd_state->stickers = bool_buf;
            if (!state_file_write(config, "stickers", (mympd_state->stickers == true ? "true" : "false")))
                response->length = snprintf(response->data, MAX_SIZE, "{\"type\": \"error\", \"data\": \"Failed to set myMPD state %%{state}\", \"values\": {\"state\": \"stickers\"}}");
        }
        je = json_scanf(request->data, request->length, "{data: {lastPlayedCount: %d}}", &int_buf1);
        if (je == 1) {
            mympd_state->last_played_count = int_buf1;
            snprintf(p_char, 7, "%d", mympd_state->last_played_count);
            if (!state_file_write(config, "last_played_count", p_char))
                response->length = snprintf(response->data, MAX_SIZE, "{\"type\": \"error\", \"data\": \"Failed to set myMPD state %%{state}\", \"values\": {\"state\": \"last_played_count\"}}");
        }
        je = json_scanf(request->data, request->length, "{data: {taglist: %Q}}", &p_charbuf1);
        if (je == 1) {
            REASSIGN_PTR(mympd_state->taglist, p_charbuf1);
            if (!state_file_write(config, "taglist", mympd_state->taglist))
                response->length = snprintf(response->data, MAX_SIZE, "{\"type\": \"error\", \"data\": \"Failed to set myMPD state %%{state}\", \"values\": {\"state\": \"taglist\"}}");
        }
        je = json_scanf(request->data, request->length, "{data: {searchtaglist: %Q}}", &p_charbuf1);
        if (je == 1) {
            REASSIGN_PTR(mympd_state->searchtaglist, p_charbuf1);
            if (!state_file_write(config, "searchtaglist", mympd_state->searchtaglist))
                response->length = snprintf(response->data, MAX_SIZE, "{\"type\": \"error\", \"data\": \"Failed to set myMPD state %%{state}\", \"values\": {\"state\": \"searchtaglist\"}}");
        }
        je = json_scanf(request->data, request->length, "{data: {browsetaglist: %Q}}", &p_charbuf1);
        if (je == 1) {
            REASSIGN_PTR(mympd_state->browsetaglist, p_charbuf1);
            if (!state_file_write(config, "browsetaglist", mympd_state->browsetaglist))
                response->length = snprintf(response->data, MAX_SIZE, "{\"type\": \"error\", \"data\": \"Failed to set myMPD state %%{state}\", \"values\": {\"state\": \"browsetaglist\"}}");
        }
        je = json_scanf(request->data, request->length, "{data: {smartpls: %B}}", &mympd_state->smartpls);
        if (je == 1) {
            if (!state_file_write(config, "smartpls", (mympd_state->smartpls == true ? "true" : "false")))
                response->length = snprintf(response->data, MAX_SIZE, "{\"type\": \"error\", \"data\": \"Failed to set myMPD state %%{state}\", \"values\": {\"state\": \"smartpls\"}}");
        }
        je = json_scanf(request->data, request->length, "{data: {maxElementsPerPage: %d}}", &int_buf1);
        if (je == 1) {
            mympd_state->max_elements_per_page = int_buf1;
            if (mympd_state->max_elements_per_page > MAX_ELEMENTS_PER_PAGE) {
                LOG_WARN("max_elements_per_page to big, setting it to maximum value of %d", MAX_ELEMENTS_PER_PAGE);
                mympd_state->max_elements_per_page = MAX_ELEMENTS_PER_PAGE;
            }
            snprintf(p_char, 7, "%d", mympd_state->max_elements_per_page);
            if (!state_file_write(config, "max_elements_per_page", p_char))
                response->length = snprintf(response->data, MAX_SIZE, "{\"type\": \"error\", \"data\": \"Failed to set myMPD state %%{state}\", \"values\": {\"state\": \"max_elements_per_page\"}}");
        }
        je = json_scanf(request->data, request->length, "{data: {love: %B}}", &bool_buf);
        if (je == 1) {
            mympd_state->love = bool_buf;
            if (!state_file_write(config, "love", (mympd_state->love == true ? "true" : "false")))
                response->length = snprintf(response->data, MAX_SIZE, "{\"type\": \"error\", \"data\": \"Failed to set myMPD state %%{state}\", \"values\": {\"state\": \"love\"}}");
        }
        je = json_scanf(request->data, request->length, "{data: {loveChannel: %Q}}", &p_charbuf1);
        if (je == 1) {
            REASSIGN_PTR(mympd_state->love_channel, p_charbuf1);
            if (!state_file_write(config, "love_channel", mympd_state->love_channel))
                response->length = snprintf(response->data, MAX_SIZE, "{\"type\": \"error\", \"data\": \"Failed to set myMPD state %%{state}\", \"values\": {\"state\": \"love_channel\"}}");
        }
        je = json_scanf(request->data, request->length, "{data: {loveMessage: %Q}}", &p_charbuf1);
        if (je == 1) {
            REASSIGN_PTR(mympd_state->love_message, p_charbuf1);
            if (!state_file_write(config, "love_message", mympd_state->love_message))
                response->length = snprintf(response->data, MAX_SIZE, "{\"type\": \"error\", \"data\": \"Failed to set myMPD state %%{state}\", \"values\": {\"state\": \"love_message\"}}");
        }
        if (response->length == 0) {
            //forward request to mpd_client queue            
            t_work_request *mpd_client_request = (t_work_request *)malloc(sizeof(t_work_request));
            assert(mpd_client_request);
            mpd_client_request->conn_id = -1;
            mpd_client_request->cmd_id = request->cmd_id;
            mpd_client_request->length = copy_string(mpd_client_request->data, request->data, 1000, request->length);
            tiny_queue_push(mpd_client_queue, mpd_client_request);
            
            response->length = snprintf(response->data, MAX_SIZE, "{\"type\": \"result\", \"data\": \"ok\"}");
        }
    }
    else if (request->cmd_id == MYMPD_API_SETTINGS_GET) {
        response->length = mympd_api_put_settings(config, mympd_state, response->data);
    }
    else if (request->cmd_id == MYMPD_API_CONNECTION_SAVE) {
        je = json_scanf(request->data, request->length, "{data: {mpdHost: %Q, mpdPort: %d, mpdPass: %Q, musicDirectory: %Q}}", 
            &p_charbuf1, &mympd_state->mpd_port, &p_charbuf2, &p_charbuf3);
        if (je == 4) {
            if (strcmp(p_charbuf2, "dontsetpassword") != 0) {
                REASSIGN_PTR(mympd_state->mpd_pass, p_charbuf1);
                if (!state_file_write(config, "mpd_pass", mympd_state->mpd_pass)) {
                    response->length = snprintf(response->data, MAX_SIZE, "{\"type\": \"error\", \"data\": \"Failed to set myMPD state %%{state}\", \"values\": {\"state\": \"mpd_pass\"}}");
                }
            }
            else {
                FREE_PTR(p_charbuf2);
            }
            REASSIGN_PTR(mympd_state->mpd_host, p_charbuf1);
            REASSIGN_PTR(mympd_state->music_directory, p_charbuf3);
            if (!state_file_write(config, "mpd_host", mympd_state->mpd_host)) {
                response->length = snprintf(response->data, MAX_SIZE, "{\"type\": \"error\", \"data\": \"Failed to set myMPD state %%{state}\", \"values\": {\"state\": \"mpd_host\"}}");
            }
            if (!state_file_write(config, "music_directory", mympd_state->music_directory)) {
                response->length = snprintf(response->data, MAX_SIZE, "{\"type\": \"error\", \"data\": \"Failed to set myMPD state %%{state}\", \"values\": {\"state\": \"music_directory\"}}");
            }
            snprintf(p_char, 7, "%d", mympd_state->mpd_port);
            if (!state_file_write(config, "mpd_port", p_char)) {
                response->length = snprintf(response->data, MAX_SIZE, "{\"type\": \"error\", \"data\": \"Failed to set myMPD state %%{state}\", \"values\": {\"state\": \"mpd_port\"}}");
            }
            //push settings to mpd_client queue
            mympd_api_push_to_mpd_client(mympd_state);
            response->length = snprintf(response->data, MAX_SIZE, "{\"type\": \"result\", \"data\": \"ok\"}");
        }
    }
    else if (request->cmd_id == MYMPD_API_BOOKMARK_SAVE) {
        je = json_scanf(request->data, request->length, "{data: {id: %d, name: %Q, uri: %Q, type: %Q}}", &int_buf1, &p_charbuf1, &p_charbuf2, &p_charbuf3);
        if (je == 4) {
            if (mympd_api_bookmark_update(config, int_buf1, p_charbuf1, p_charbuf2, p_charbuf3)) {
                response->length = snprintf(response->data, MAX_SIZE, "{\"type\": \"result\", \"data\": \"ok\"}");
            }
            else {
                response->length = snprintf(response->data, MAX_SIZE, "{\"type\": \"error\", \"data\": \"Saving bookmark failed\"}");
            }
            FREE_PTR(p_charbuf1);
            FREE_PTR(p_charbuf2);
            FREE_PTR(p_charbuf3);
        }
    }
    else if (request->cmd_id == MYMPD_API_BOOKMARK_RM) {
        je = json_scanf(request->data, request->length, "{data: {id: %d}}", &int_buf1);
        if (je == 1) {
            if (mympd_api_bookmark_update(config, int_buf1, NULL, NULL, NULL)) {
                response->length = snprintf(response->data, MAX_SIZE, "{\"type\": \"result\", \"data\": \"ok\"}");
            }
            else {
                response->length = snprintf(response->data, MAX_SIZE, "{\"type\": \"error\", \"data\": \"Deleting bookmark failed\"}");
            }
        }
    }
    else if (request->cmd_id == MYMPD_API_BOOKMARK_LIST) {
        je = json_scanf(request->data, request->length, "{data: {offset: %u}}", &uint_buf1);
        if (je == 1) {
            response->length = mympd_api_bookmark_list(config, response->data, uint_buf1);
        }
    }
    else {
        response->length = snprintf(response->data, MAX_SIZE, "{\"type\": \"error\", \"data\": \"Unknown request\"}");
        LOG_ERROR("Unknown cmd_id %u", request->cmd_id);    
    }

    if (response->length == 0) {
        response->length = snprintf(response->data, MAX_SIZE, "{\"type\": \"error\", \"data\": \"No response for cmd_id %%{cmdId}\", \"values\": {\"cmdId\": %d}}", request->cmd_id);
        LOG_ERROR("No response for cmd_id %u", request->cmd_id);
    }
    LOG_DEBUG("Push response to queue for connection %lu: %s", request->conn_id, response->data);

    tiny_queue_push(web_server_queue, response);
    FREE_PTR(request);
}

static int mympd_api_syscmd(t_config *config, char *buffer, const char *cmd) {
    int len = 0;
    
    char *cmdline = (char *)list_get_extra(&config->syscmd_list, cmd);
    if (cmdline == NULL) {
        LOG_ERROR("Syscmd not defined: %s", cmd);
        len = snprintf(buffer, MAX_SIZE, "{\"type\": \"error\", \"data\": \"System command not defined\"}");
        return len;
    }

    const int rc = system(cmdline);
    if ( rc == 0) {
        len = snprintf(buffer, MAX_SIZE, "{\"type\": \"result\", \"data\": \"Successfully execute cmd %%{cmd}\", \"values\":{\"cmd\": \"%s\"}}", cmd);
        LOG_VERBOSE("Executed syscmd: \"%s\"", cmdline);
    }
    else {
        len = snprintf(buffer, MAX_SIZE, "{\"type\": \"error\", \"data\": \"Failed to execute cmd %%{cmd}\", \"values\":{\"cmd\": \"%s\"}}", cmd);
        LOG_ERROR("Executing syscmd \"%s\" failed", cmdline);
    }
    CHECK_RETURN_LEN();    
}

static void mympd_api_settings_reset(t_config *config, t_mympd_state *mympd_state) {

    mympd_api_settings_delete(config);
    mympd_api_read_statefiles(config, mympd_state);
    mympd_api_push_to_mpd_client(mympd_state);
}

static void mympd_api_read_statefiles(t_config *config, t_mympd_state *mympd_state) {
    LOG_INFO("Reading states");
    mympd_state->mpd_host = state_file_rw_string(config, "mpd_host", config->mpd_host, false);
    mympd_state->mpd_port = state_file_rw_int(config, "mpd_port", config->mpd_port, false);
    mympd_state->mpd_pass = state_file_rw_string(config, "mpd_pass", config->mpd_pass, false);
    mympd_state->stickers = state_file_rw_bool(config, "stickers", config->stickers, false);
    mympd_state->taglist = state_file_rw_string(config, "taglist", config->taglist, false);
    mympd_state->searchtaglist = state_file_rw_string(config, "searchtaglist", config->searchtaglist, false);
    mympd_state->browsetaglist = state_file_rw_string(config, "browsetaglist", config->browsetaglist, false);
    mympd_state->smartpls = state_file_rw_bool(config, "smartpls", config->smartpls, false);
    mympd_state->max_elements_per_page = state_file_rw_int(config, "max_elements_per_page", config->max_elements_per_page, false);
    mympd_state->last_played_count = state_file_rw_int(config, "last_played_count", config->last_played_count, false);
    mympd_state->love = state_file_rw_bool(config, "love", config->love, false);
    mympd_state->love_channel = state_file_rw_string(config, "love_channel", config->love_channel, false);
    mympd_state->love_message = state_file_rw_string(config, "love_message", config->love_message, false);
    mympd_state->notification_web = state_file_rw_bool(config, "notification_web", config->notification_web, false);
    mympd_state->notification_page = state_file_rw_bool(config, "notification_page", config->notification_page, false);
    mympd_state->auto_play = state_file_rw_bool(config, "auto_play", config->auto_play, false);
    mympd_state->jukebox_mode = state_file_rw_int(config, "jukebox_mode", config->jukebox_mode, false);
    mympd_state->jukebox_playlist = state_file_rw_string(config, "jukebox_playlist", config->jukebox_playlist, false);
    mympd_state->jukebox_queue_length = state_file_rw_int(config, "jukebox_queue_length", config->jukebox_queue_length, false);
    mympd_state->cols_queue_current = state_file_rw_string(config, "cols_queue_current", config->cols_queue_current, false);
    mympd_state->cols_search = state_file_rw_string(config, "cols_search", config->cols_search, false);
    mympd_state->cols_browse_database = state_file_rw_string(config, "cols_browse_database", config->cols_browse_database, false);
    mympd_state->cols_browse_playlists_detail = state_file_rw_string(config, "cols_browse_playlists_detail", config->cols_browse_playlists_detail, false);
    mympd_state->cols_browse_filesystem = state_file_rw_string(config, "cols_browse_filesystem", config->cols_browse_filesystem, false);
    mympd_state->cols_playback = state_file_rw_string(config, "cols_playback", config->cols_playback, false);
    mympd_state->cols_queue_last_played = state_file_rw_string(config, "cols_queue_last_played", config->cols_queue_last_played, false);
    mympd_state->localplayer = state_file_rw_bool(config, "localplayer", config->localplayer, false);
    mympd_state->localplayer_autoplay = state_file_rw_bool(config, "localplayer_autoplay", config->localplayer_autoplay, false);
    mympd_state->stream_port = state_file_rw_int(config, "stream_port", config->stream_port, false);
    mympd_state->stream_url = state_file_rw_string(config, "stream_url", config->stream_url, false);
    mympd_state->bg_cover = state_file_rw_bool(config, "bg_cover", config->bg_cover, false);
    mympd_state->bg_color = state_file_rw_string(config, "bg_color", config->bg_color, false);
    mympd_state->bg_css_filter = state_file_rw_string(config, "bg_css_filter", config->bg_css_filter, false);
    mympd_state->coverimage = state_file_rw_bool(config, "coverimage", config->coverimage, false);
    mympd_state->coverimage_name = state_file_rw_string(config, "coverimage_name", config->coverimage_name, false);
    mympd_state->coverimage_size = state_file_rw_int(config, "coverimage_size", config->coverimage_size, false);
    mympd_state->locale = state_file_rw_string(config, "locale", config->locale, false);
    mympd_state->music_directory = state_file_rw_string(config, "music_directory", config->music_directory, false);
}

static char *state_file_rw_string(t_config *config, const char *name, const char *def_value, bool warn) {
    char *line = NULL;
    size_t n = 0;
    ssize_t read;
    
    if (!validate_string(name)) {
        return NULL;
    }
    size_t cfg_file_len = config->varlibdir_len + strlen(name) + 8;
    char cfg_file[cfg_file_len];
    snprintf(cfg_file, cfg_file_len, "%s/state/%s", config->varlibdir, name);
    FILE *fp = fopen(cfg_file, "r");
    if (fp == NULL) {
        if (warn == true) {
            LOG_WARN("Can't open %s", cfg_file);
        }
        state_file_write(config, name, def_value);
        return strdup(def_value);
    }
    read = getline(&line, &n, fp);
    if (read > 0) {
        LOG_DEBUG("State %s: %s", name, line);
    }
    fclose(fp);
    if (read > 0) {
        return line;
    }
    else {
        FREE_PTR(line);
        return strdup(def_value);
    }
}

static bool state_file_rw_bool(t_config *config, const char *name, const bool def_value, bool warn) {
    bool value = def_value;
    char *line = state_file_rw_string(config, name, def_value == true ? "true" : "false", warn);
    if (line != NULL) {
        value = strcmp(line, "true") == 0 ? true : false;
        FREE_PTR(line);
    }
    return value;
}

static int state_file_rw_int(t_config *config, const char *name, const int def_value, bool warn) {
    char *crap = NULL;
    int value = def_value;
    char def_value_str[7];
    snprintf(def_value_str, 7, "%d", def_value);
    char *line = state_file_rw_string(config, name, def_value_str, warn);
    if (line != NULL) {
        value = strtoimax(line, &crap, 10);
        FREE_PTR(line);
    }
    return value;
}

static bool state_file_write(t_config *config, const char *name, const char *value) {
    size_t cfg_file_len = config->varlibdir_len + strlen(name) + 8;
    char cfg_file[cfg_file_len];
    size_t tmp_file_len = config->varlibdir_len + strlen(name) + 15;
    char tmp_file[tmp_file_len];
    int fd;
    
    if (!validate_string(name))
        return false;
    snprintf(cfg_file, cfg_file_len, "%s/state/%s", config->varlibdir, name);
    snprintf(tmp_file, tmp_file_len, "%s/state/%s.XXXXXX", config->varlibdir, name);
        
    if ((fd = mkstemp(tmp_file)) < 0 ) {
        LOG_ERROR("Can't open %s for write", tmp_file);
        return false;
    }
    FILE *fp = fdopen(fd, "w");
    fprintf(fp, "%s", value);
    fclose(fp);
    if (rename(tmp_file, cfg_file) == -1) {
        LOG_ERROR("Renaming file from %s to %s failed", tmp_file, cfg_file);
        return false;
    }
    return true;
}

static int mympd_api_put_settings(t_config *config, t_mympd_state *mympd_state, char *buffer) {
    size_t len;
    struct json_out out = JSON_OUT_BUF(buffer, MAX_SIZE);
    
    len = json_printf(&out, "{type: mympdSettings, data: {mpdHost: %Q, mpdPort: %d, mpdPass: %Q, featSyscmds: %B, featCacert: %B, "
        "featLocalplayer: %B, streamPort: %d, streamUrl: %Q, coverimage: %B, coverimageName: %Q, coverimageSize: %d, featMixramp: %B, "
        "maxElementsPerPage: %d, notificationWeb: %B, notificationPage: %B, jukeboxMode: %d, jukeboxPlaylist: %Q, jukeboxQueueLength: %d, "
        "autoPlay: %B, bgColor: %Q, bgCover: %B, bgCssFilter: %Q, loglevel: %d, locale: %Q, localplayerAutoplay: %B, "
        "stickers: %B, smartpls: %B, lastPlayedCount: %d, love: %B, loveChannel: %Q, loveMessage: %Q, musicDirectory: %Q",
        mympd_state->mpd_host, 
        mympd_state->mpd_port,
        "dontsetpassword",
        config->syscmds,
        (config->custom_cert == false && config->ssl == true ? true : false),
        mympd_state->localplayer,
        mympd_state->stream_port,
        mympd_state->stream_url,
        mympd_state->coverimage,
        mympd_state->coverimage_name,
        mympd_state->coverimage_size,
        config->mixramp,
        mympd_state->max_elements_per_page,
        mympd_state->notification_web,
        mympd_state->notification_page,
        mympd_state->jukebox_mode,
        mympd_state->jukebox_playlist,
        mympd_state->jukebox_queue_length,
        mympd_state->auto_play,
        mympd_state->bg_color,
        mympd_state->bg_cover,
        mympd_state->bg_css_filter,
        loglevel,
        mympd_state->locale,
        mympd_state->localplayer_autoplay,
        mympd_state->stickers,
        mympd_state->smartpls,
        mympd_state->last_played_count,
        mympd_state->love,
        mympd_state->love_channel,
        mympd_state->love_message,
        mympd_state->music_directory
    );
    
    if (config->syscmds == true) {
        len += json_printf(&out, ", syscmdList: [");
        int nr = 0;
        struct node *current = config->syscmd_list.list;
        while (current != NULL) {
            if (nr++) 
                len += json_printf(&out, ",");
            len += json_printf(&out, "%Q", current->data);
            current = current->next;
        }
        len += json_printf(&out, "]");
    }
    len += json_printf(&out, ", colsQueueCurrent: %s, colsSearch: %s, colsBrowseDatabase: %s, colsBrowsePlaylistsDetail: %s, "
        "colsBrowseFilesystem: %s, colsPlayback: %s, colsQueueLastPlayed: %s}}",
        mympd_state->cols_queue_current,
        mympd_state->cols_search,
        mympd_state->cols_browse_database,
        mympd_state->cols_browse_playlists_detail,
        mympd_state->cols_browse_filesystem,
        mympd_state->cols_playback,
        mympd_state->cols_queue_last_played
    );

    CHECK_RETURN_LEN();
}

static bool mympd_api_bookmark_update(t_config *config, const int id, const char *name, const char *uri, const char *type) {
    int line_nr = 0;
    size_t tmp_file_len = config->varlibdir_len + 24;
    char tmp_file[tmp_file_len];
    size_t b_file_len = config->varlibdir_len + 17;
    char b_file[b_file_len];
    char *line = NULL;
    size_t n = 0;
    ssize_t read;
    bool inserted = false;
    int fd;
    
    snprintf(b_file, b_file_len, "%s/state/bookmarks", config->varlibdir);
    snprintf(tmp_file, tmp_file_len, "%s/state/bookmarks.XXXXXX", config->varlibdir);
    
    if ((fd = mkstemp(tmp_file)) < 0 ) {
        LOG_ERROR("Can't open %s for write", tmp_file);
        return false;
    }
    FILE *fo = fdopen(fd, "w");
    FILE *fi = fopen(b_file, "r");
    if (fi != NULL) {
        while ((read = getline(&line, &n, fi)) > 0) {
            char *lname = NULL;
            char *luri = NULL;
            char *ltype = NULL;
            int lid;
            int je = json_scanf(line, read, "{id: %d, name: %Q, uri: %Q, type: %Q}", &lid, &lname, &luri, &ltype);
            if (je == 4) {
                if (name != NULL) {
                    if (strcmp(name, lname) < 0) {
                        line_nr++;
                        struct json_out out = JSON_OUT_FILE(fo);
                        json_printf(&out, "{id: %d, name: %Q, uri: %Q, type: %Q}\n", line_nr, name, uri, type);
                        inserted = true;
                    }
                }
                if (lid != id) {
                    line_nr++;
                    struct json_out out = JSON_OUT_FILE(fo);
                    json_printf(&out, "{id: %d, name: %Q, uri: %Q, type: %Q}\n", line_nr, lname, luri, ltype);
                }
                FREE_PTR(lname);
                FREE_PTR(luri);
                FREE_PTR(ltype);
            }
            else {
                LOG_ERROR("Can't read bookmarks line");
            }
        }
        FREE_PTR(line);
        fclose(fi);
    }
    if (inserted == false && name != NULL) {
        line_nr++;
        struct json_out out = JSON_OUT_FILE(fo);
        json_printf(&out, "{id: %d, name: %Q, uri: %Q, type: %Q}\n", line_nr, name, uri, type);
    }
    fclose(fo);
    if (rename(tmp_file, b_file) == -1) {
        LOG_ERROR("Rename file from %s to %s failed", tmp_file, b_file);
        return false;
    }
    return true;
}

static int mympd_api_bookmark_list(t_config *config, char *buffer, unsigned int offset) {
    size_t len = 0;
    size_t b_file_len = strlen(config->varlibdir) + 17;
    char b_file[b_file_len];
    char *line = NULL;
    char *crap = NULL;
    size_t n = 0;
    ssize_t read;
    unsigned entity_count = 0;
    unsigned entities_returned = 0;
    struct json_out out = JSON_OUT_BUF(buffer, MAX_SIZE);
    
    snprintf(b_file, b_file_len, "%s/state/bookmarks", config->varlibdir);
    FILE *fi = fopen(b_file, "r");
    if (fi == NULL) {
        //create empty bookmarks file
        fi = fopen(b_file, "w");
        if (fi == NULL) {
            LOG_ERROR("Can't open %s for write", b_file);
            len = json_printf(&out, "{type: error, data: %Q}", "Failed to open bookmarks file");
        }
        else {
            fclose(fi);
            len = json_printf(&out, "{type: bookmark, data: [], totalEntities: 0, offset: 0, returnedEntities: 0}");
        }
    } else {
        len = json_printf(&out, "{type: bookmark, data: [");
        while ((read = getline(&line, &n, fi)) > 0 && len < MAX_LIST_SIZE) {
            entity_count++;
            if (entity_count > offset && entity_count <= offset + config->max_elements_per_page) {
                if (entities_returned++) {
                    len += json_printf(&out, ",");
                }
                strtok_r(line, "\n", &crap);
                len += json_printf(&out, "%s", line);
            }
        }
        FREE_PTR(line);
        fclose(fi);
        len += json_printf(&out, "], totalEntities: %d, offset: %d, returnedEntities: %d}",
            entity_count,
            offset,
            entities_returned
        );
    }
    CHECK_RETURN_LEN();
}
