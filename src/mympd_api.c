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

#include "list.h"
#include "tiny_queue.h"
#include "global.h"
#include "mympd_api.h"
#include "mpd_client.h"
#include "../dist/src/frozen/frozen.h"

//private definitions
typedef struct t_mympd_state {
    //notifications
    bool notification_web;
    bool notification_page;

    //jukebox
    enum jukebox_modes jukebox_mode;
    char *jukebox_playlist;
    int jukebox_queue_length;

    bool auto_play;
    bool feat_localplayer;
    bool localplayer_autoplay;
    int stream_port;
    char *stream_url;
    bool bg_cover;
    char *bg_color;
    char *bg_css_filter;
    char *locale;
    bool coverimage;
    char *coverimage_name;
    int coverimage_size;

    //columns
    char *cols_queue_current;
    char *cols_search;
    char *cols_browse_database;
    char *cols_browse_playlists_detail;
    char *cols_browse_filesystem;
    char *cols_playback;
    char *cols_queue_last_played;
    
    //system commands
    struct list syscmd_list;
} t_mympd_state;

static void mympd_api(t_config *config, t_mympd_state *mympd_state, t_work_request *request);
static bool mympd_api_read_syscmds(t_config *config, t_mympd_state *mympd_state);
static int mympd_api_syscmd(t_config *config, t_mympd_state *mympd_state, char *buffer, const char *cmd);
static void mympd_api_read_statefiles(t_config *config, t_mympd_state *mympd_state);
static char *state_file_rw_string(t_config *config, const char *name, const char *def_value, bool warn);
static bool state_file_rw_bool(t_config *config, const char *name, const bool def_value, bool warn);
static long state_file_rw_long(t_config *config, const char *name, const long def_value, bool warn);
static bool state_file_write(t_config *config, const char *name, const char *value);
static int mympd_api_put_settings(t_config *config, t_mympd_state *mympd_state, char *buffer);
static bool mympd_api_bookmark_update(t_config *config, const long id, const char *name, const char *uri, const char *type);
static int mympd_api_bookmark_list(t_config *config, char *buffer, unsigned int offset);

//public functions
void *mympd_api_loop(void *arg_config) {
    t_config *config = (t_config *) arg_config;
    
    //read myMPD states under config.varlibdir
    t_mympd_state mympd_state;
    mympd_api_read_statefiles(config, &mympd_state);

    //push settings to mpd_client queue
    t_work_request *mpd_client_request = (t_work_request *)malloc(sizeof(t_work_request));
    assert(mpd_client_request);
    struct json_out out = JSON_OUT_BUF(mpd_client_request->data, 1000);
    mpd_client_request->conn_id = -1;
    mpd_client_request->cmd_id = MYMPD_API_SETTINGS_SET;
    mpd_client_request->length = json_printf(&out, "{cmd: MYMPD_API_SETTINGS_SET, data: {jukeboxMode: %d, jukeboxPlaylist: %Q, jukeboxQueueLength: %d, "
        "autoPlay: %B, coverimageName: %Q}}",
        mympd_state.jukebox_mode,
        mympd_state.jukebox_playlist,
        mympd_state.jukebox_queue_length,
        mympd_state.auto_play,
        mympd_state.coverimage_name
    );
    tiny_queue_push(mpd_client_queue, mpd_client_request);

    //read system command files
    list_init(&mympd_state.syscmd_list);
    bool rc = mympd_api_read_syscmds(config, &mympd_state);
    if (rc == true) {
        list_sort_by_value(&mympd_state.syscmd_list, true);
    }

    while (s_signal_received == 0) {
        struct t_work_request *request = tiny_queue_shift(mympd_api_queue, 0);
        if (request != NULL) {
            mympd_api(config, &mympd_state, request);
        }
    }

    list_free(&mympd_state.syscmd_list);
    FREE_PTR(mympd_state.jukebox_playlist);
    FREE_PTR(mympd_state.cols_queue_current);
    FREE_PTR(mympd_state.cols_search);
    FREE_PTR(mympd_state.cols_browse_database);
    FREE_PTR(mympd_state.cols_browse_playlists_detail);
    FREE_PTR(mympd_state.cols_browse_filesystem);
    FREE_PTR(mympd_state.cols_playback);
    FREE_PTR(mympd_state.cols_queue_last_played);
    FREE_PTR(mympd_state.stream_url);
    FREE_PTR(mympd_state.bg_color);
    FREE_PTR(mympd_state.bg_css_filter);
    FREE_PTR(mympd_state.coverimage_name);
    FREE_PTR(mympd_state.locale);
    return NULL;
}

//private functions
static void mympd_api(t_config *config, t_mympd_state *mympd_state, t_work_request *request) {
    int je;
    char *p_charbuf1 = NULL;
    char *p_charbuf2 = NULL;
    char *p_charbuf3 = NULL;
    char p_char[7];
    long long_buf1;
    unsigned int uint_buf1;
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
                response->length = mympd_api_syscmd(config, mympd_state, response->data, p_charbuf1);
                FREE_PTR(p_charbuf1);
            }
        } 
        else {
            response->length = snprintf(response->data, MAX_SIZE, "{\"type\": \"error\", \"data\": \"System commands are disabled.\"}");
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
                response->length = snprintf(response->data, MAX_SIZE, "{\"type\": \"error\", \"data\": \"Unknown table %s\"}", p_charbuf1);
                LOG_ERROR("MYMPD_API_COLS_SAVE: Unknown table %s", p_charbuf1);
            }
            if (response->length == 0) {
                if (state_file_write(config, p_charbuf1, cols))
                    response->length = snprintf(response->data, MAX_SIZE, "{\"type\": \"result\", \"data\": \"ok\"}");
            }
            FREE_PTR(p_charbuf1);
        }
    }
    else if (request->cmd_id == MYMPD_API_SETTINGS_SET) {
        je = json_scanf(request->data, request->length, "{data: {notificationWeb: %B}}", &mympd_state->notification_web);
        if (je == 1) {
            if (!state_file_write(config, "notification_web", (mympd_state->notification_web == true ? "true" : "false")))
                response->length = snprintf(response->data, MAX_SIZE, "{\"type\": \"error\", \"data\": \"Can't set state notification_web.\"}");
        }    
        je = json_scanf(request->data, request->length, "{data: {notificationPage: %B}}", &mympd_state->notification_page);
        if (je == 1) {
            if (!state_file_write(config, "notification_page", (mympd_state->notification_page == true ? "true" : "false")))
                response->length = snprintf(response->data, MAX_SIZE, "{\"type\": \"error\", \"data\": \"Can't set state notification_page.\"}");
        }
        je = json_scanf(request->data, request->length, "{data: {autoPlay: %B}}", &mympd_state->auto_play);
        if (je == 1) {
            if (!state_file_write(config, "auto_play", (mympd_state->auto_play == true ? "true" : "false")))
                response->length = snprintf(response->data, MAX_SIZE, "{\"type\": \"error\", \"data\": \"Can't set state auto_play.\"}");
        }
        je = json_scanf(request->data, request->length, "{data: {localplayerAutoplay: %B}}", &mympd_state->localplayer_autoplay);
        if (je == 1) {
            if (!state_file_write(config, "localplayer_autoplay", (mympd_state->localplayer_autoplay == true ? "true" : "false")))
                response->length = snprintf(response->data, MAX_SIZE, "{\"type\": \"error\", \"data\": \"Can't set state localplayer_autoplay.\"}");
        }
        je = json_scanf(request->data, request->length, "{data: {coverimage: %B}}", &mympd_state->coverimage);
        if (je == 1) {
            if (!state_file_write(config, "coverimage", (mympd_state->coverimage == true ? "true" : "false")))
                response->length = snprintf(response->data, MAX_SIZE, "{\"type\": \"error\", \"data\": \"Can't set state coverimage.\"}");
        }
        
        je = json_scanf(request->data, request->length, "{data: {coverimageName: %Q}}", &p_charbuf1);
        if (je == 1) {
            if (validate_string(p_charbuf1) && strlen(p_charbuf1) > 0) {
                FREE_PTR(mympd_state->coverimage_name);
                mympd_state->coverimage_name = p_charbuf1;
                p_charbuf1 = NULL;
                if (!state_file_write(config, "coverimage_name", mympd_state->coverimage_name))
                    response->length = snprintf(response->data, MAX_SIZE, "{\"type\": \"error\", \"data\": \"Can't set state coverimage_name.\"}");
            }
            else {
                FREE_PTR(p_charbuf1);
                response->length = snprintf(response->data, MAX_SIZE, "{\"type\": \"error\", \"data\": \"Invalid filename for coverimage_name.\"}");
            }
        }
        
        je = json_scanf(request->data, request->length, "{data: {coverimageSize: %d}}", &mympd_state->coverimage_size);
        if (je == 1) {
            snprintf(p_char, 7, "%d", mympd_state->coverimage_size);
            if (!state_file_write(config, "coverimage_size", p_char))
                response->length = snprintf(response->data, MAX_SIZE, "{\"type\": \"error\", \"data\": \"Can't set state coverimage_size.\"}");
        }
        je = json_scanf(request->data, request->length, "{data: {featLocalplayer: %B}}", &mympd_state->feat_localplayer);
        if (je == 1) {
            if (!state_file_write(config, "feat_localplayer", (mympd_state->feat_localplayer == true ? "true" : "false")))
                response->length = snprintf(response->data, MAX_SIZE, "{\"type\": \"error\", \"data\": \"Can't set state feat_localplayer.\"}");
        }
        je = json_scanf(request->data, request->length, "{data: {streamPort: %d}}", &mympd_state->stream_port);
        if (je == 1) {
            snprintf(p_char, 7, "%d", mympd_state->stream_port);
            if (!state_file_write(config, "stream_port", p_char))
                response->length = snprintf(response->data, MAX_SIZE, "{\"type\": \"error\", \"data\": \"Can't set state stream_port.\"}");
        }
        je = json_scanf(request->data, request->length, "{data: {streamUrl: %Q}}", &p_charbuf1);
        if (je == 1) {
            FREE_PTR(mympd_state->stream_url);
            mympd_state->stream_url = p_charbuf1;
            p_charbuf1 = NULL;
            if (!state_file_write(config, "stream_url", mympd_state->stream_url))
                response->length = snprintf(response->data, MAX_SIZE, "{\"type\": \"error\", \"data\": \"Can't set state stream_url.\"}");
        }
        je = json_scanf(request->data, request->length, "{data: {locale: %Q}}", &p_charbuf1);
        if (je == 1) {
            FREE_PTR(mympd_state->locale);
            mympd_state->locale = p_charbuf1;
            p_charbuf1 = NULL;
            if (!state_file_write(config, "locale", mympd_state->locale))
                response->length = snprintf(response->data, MAX_SIZE, "{\"type\": \"error\", \"data\": \"Can't set state locale.\"}");
        }
        je = json_scanf(request->data, request->length, "{data: {bgCover: %B}}", &mympd_state->bg_cover);
        if (je == 1) {
            if (!state_file_write(config, "bg_cover", (mympd_state->bg_cover == true ? "true" : "false")))
                response->length = snprintf(response->data, MAX_SIZE, "{\"type\": \"error\", \"data\": \"Can't set state bg_cover.\"}");
        }
        je = json_scanf(request->data, request->length, "{data: {bgColor: %Q}}", &p_charbuf1);
        if (je == 1) {
            FREE_PTR(mympd_state->bg_color);
            mympd_state->bg_color = p_charbuf1;
            p_charbuf1 = NULL;
            if (!state_file_write(config, "bg_color", mympd_state->bg_color))
                response->length = snprintf(response->data, MAX_SIZE, "{\"type\": \"error\", \"data\": \"Can't set state bg_color.\"}");
        }
        je = json_scanf(request->data, request->length, "{data: {bgCssFilter: %Q}}", &p_charbuf1);
        if (je == 1) {
            FREE_PTR(mympd_state->bg_css_filter);
            mympd_state->bg_css_filter = p_charbuf1;
            p_charbuf1 = NULL;
            if (!state_file_write(config, "bg_css_filter", mympd_state->bg_css_filter))
                response->length = snprintf(response->data, MAX_SIZE, "{\"type\": \"error\", \"data\": \"Can't set state bg_css_filter.\"}");
        }
        je = json_scanf(request->data, request->length, "{data: {jukeboxMode: %d}}", &mympd_state->jukebox_mode);
        if (je == 1) {
            if (mympd_state->jukebox_mode > 2) {
                response->length = snprintf(response->data, MAX_SIZE, "{\"type\": \"error\", \"data\": \"Invalid jukeboxMode.\"}");
                LOG_ERROR("Invalid jukeboxMode");
                mympd_state->jukebox_mode = JUKEBOX_OFF;
            }
            snprintf(p_char, 7, "%d", mympd_state->jukebox_mode);
            if (!state_file_write(config, "jukebox_mode", p_char))
                response->length = snprintf(response->data, MAX_SIZE, "{\"type\": \"error\", \"data\": \"Can't set state jukebox_mode.\"}");
        }
        je = json_scanf(request->data, request->length, "{data: {jukeboxPlaylist: %Q}}", &p_charbuf1);
        if (je == 1) {
            FREE_PTR(mympd_state->jukebox_playlist);
            mympd_state->jukebox_playlist = p_charbuf1;
            p_charbuf1 = NULL;
            if (!state_file_write(config, "jukebox_playlist", mympd_state->jukebox_playlist))
                response->length = snprintf(response->data, MAX_SIZE, "{\"type\": \"error\", \"data\": \"Can't set state jukebox_playlist.\"}");
        }
        je = json_scanf(request->data, request->length, "{data: {jukeboxQueueLength: %d}}", &mympd_state->jukebox_queue_length);
        if (je == 1) {
            if (mympd_state->jukebox_queue_length > 999) {
                LOG_ERROR("jukeboxQueueLength to big, setting it to maximum value of 999");
                response->length = snprintf(response->data, MAX_SIZE, "{\"type\": \"error\", \"data\": \"jukeboxQueueLength to big, setting it to maximum value of 999\"}");
                mympd_state->jukebox_queue_length = 999;
            }
            snprintf(p_char, 7, "%d", mympd_state->jukebox_queue_length);
            if (!state_file_write(config, "jukebox_queue_length", p_char))
                response->length = snprintf(response->data, MAX_SIZE, "{\"type\": \"error\", \"data\": \"Can't set state jukebox_queue_length.\"}");
        }
        if (response->length == 0) {
            response->length = snprintf(response->data, MAX_SIZE, "{\"type\": \"result\", \"data\": \"ok\"}");
        }
        //push settings to mpd_client queue
        t_work_request *mpd_client_request = (t_work_request *)malloc(sizeof(t_work_request));
        assert(mpd_client_request);
        mpd_client_request->conn_id = -1;
        mpd_client_request->cmd_id = request->cmd_id;
        mpd_client_request->length = copy_string(mpd_client_request->data, request->data, 1000, request->length);
        tiny_queue_push(mpd_client_queue, mpd_client_request);
    }
    else if (request->cmd_id == MYMPD_API_SETTINGS_GET) {
        response->length = mympd_api_put_settings(config, mympd_state, response->data);
    }
    else if (request->cmd_id == MYMPD_API_BOOKMARK_SAVE) {
        je = json_scanf(request->data, request->length, "{data: {id: %ld, name: %Q, uri: %Q, type: %Q}}", &long_buf1, &p_charbuf1, &p_charbuf2, &p_charbuf3);
        if (je == 4) {
            if (mympd_api_bookmark_update(config, long_buf1, p_charbuf1, p_charbuf2, p_charbuf3)) {
                response->length = snprintf(response->data, MAX_SIZE, "{\"type\": \"result\", \"data\": \"ok\"}");
            }
            else {
                response->length = snprintf(response->data, MAX_SIZE, "{\"type\": \"error\", \"data\": \"Can't save bookmark.\"}");
            }
            FREE_PTR(p_charbuf1);
            FREE_PTR(p_charbuf2);
            FREE_PTR(p_charbuf3);
        }
    }
    else if (request->cmd_id == MYMPD_API_BOOKMARK_RM) {
        je = json_scanf(request->data, request->length, "{data: {id: %ld}}", &long_buf1);
        if (je == 1) {
            if (mympd_api_bookmark_update(config, long_buf1, NULL, NULL, NULL)) {
                response->length = snprintf(response->data, MAX_SIZE, "{\"type\": \"result\", \"data\": \"ok\"}");
            }
            else {
                response->length = snprintf(response->data, MAX_SIZE, "{\"type\": \"error\", \"data\": \"Can't delete bookmark.\"}");
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
        response->length = snprintf(response->data, MAX_SIZE, "{\"type\": \"error\", \"data\": \"Unknown cmd_id %u.\"}", request->cmd_id);
        LOG_ERROR("Unknown cmd_id %u", request->cmd_id);    
    }

    if (response->length == 0) {
        response->length = snprintf(response->data, MAX_SIZE, "{\"type\": \"error\", \"data\": \"No response for cmd_id %u.\"}", request->cmd_id);
        LOG_ERROR("No response for cmd_id %u", request->cmd_id);
    }
    LOG_DEBUG("Push response to queue for connection %lu: %s", request->conn_id, response->data);

    tiny_queue_push(web_server_queue, response);
    FREE_PTR(request);
}

static bool mympd_api_read_syscmds(t_config *config, t_mympd_state *mympd_state) {
    DIR *dir;
    struct dirent *ent;
    char dirname[400];
    char *cmd = NULL;
    long order;

    if (config->syscmds == true) {
        snprintf(dirname, 400, "%s/syscmds", config->etcdir);
        LOG_INFO("Reading syscmds: %s", dirname);
        if ((dir = opendir (dirname)) != NULL) {
            while ((ent = readdir(dir)) != NULL) {
                if (strncmp(ent->d_name, ".", 1) == 0)
                    continue;
                order = strtol(ent->d_name, &cmd, 10);
                if (strcmp(cmd, "") != 0) {
                    list_push(&mympd_state->syscmd_list, cmd, order);
                }
                else {
                    LOG_ERROR("Can't read syscmd file %s", ent->d_name);
                }
            }
            closedir(dir);
        }
        else {
            LOG_ERROR("Can't read syscmds");
        }
    }
    else {
        LOG_WARN("Syscmds are disabled");
    }
    return true;
}

static int mympd_api_syscmd(t_config *config, t_mympd_state *mympd_state, char *buffer, const char *cmd) {
    int len = 0;
    char filename[400];
    char *line = NULL;
    char *crap = NULL;
    size_t n = 0;
    ssize_t read;
    
    const int order = list_get_value(&mympd_state->syscmd_list, cmd);
    if (order == -1) {
        LOG_ERROR("Syscmd not defined: %s", cmd);
        len = snprintf(buffer, MAX_SIZE, "{\"type\": \"error\", \"data\": \"System command not defined\"}");
        return len;
    }
    
    snprintf(filename, 400, "%s/syscmds/%d%s", config->etcdir, order, cmd);
    FILE *fp = fopen(filename, "r");    
    if (fp == NULL) {
        len = snprintf(buffer, MAX_SIZE, "{\"type\": \"error\", \"data\": \"Can't execute cmd %s.\"}", cmd);
        LOG_ERROR("Can't execute syscmd \"%s\"", cmd);
        return len;
    }
    read = getline(&line, &n, fp);
    fclose(fp);
    if (read > 0) {
        strtok_r(line, "\n", &crap);
        const int rc = system(line);
        if ( rc == 0) {
            len = snprintf(buffer, MAX_SIZE, "{\"type\": \"result\", \"data\": \"Executed cmd %s.\"}", cmd);
            LOG_VERBOSE("Executed syscmd: \"%s\"", line);
        }
        else {
            len = snprintf(buffer, MAX_SIZE, "{\"type\": \"error\", \"data\": \"Executing cmd %s failed.\"}", cmd);
            LOG_ERROR("ERROR: Executing syscmd \"%s\" failed", cmd);
        }
    } else {
        len = snprintf(buffer, MAX_SIZE, "{\"type\": \"error\", \"data\": \"Can't execute cmd %s.\"}", cmd);
        LOG_ERROR("Can't execute syscmd \"%s\"", cmd);
    }
    FREE_PTR(line);
    CHECK_RETURN_LEN();    
}

static void mympd_api_read_statefiles(t_config *config, t_mympd_state *mympd_state) {
    LOG_INFO("Reading states");
    mympd_state->notification_web = state_file_rw_bool(config, "notification_web", false, false);
    mympd_state->notification_page = state_file_rw_bool(config, "notification_page", true, false);
    mympd_state->auto_play = state_file_rw_bool(config, "auto_play", false, false);
    mympd_state->jukebox_mode = state_file_rw_long(config, "jukebox_mode", JUKEBOX_OFF, false);
    mympd_state->jukebox_playlist = state_file_rw_string(config, "jukebox_playlist", "Database", false);
    mympd_state->jukebox_queue_length = state_file_rw_long(config, "jukebox_queue_length", 1, false);
    mympd_state->cols_queue_current = state_file_rw_string(config, "cols_queue_current", "[\"Pos\",\"Title\",\"Artist\",\"Album\",\"Duration\"]", false);
    mympd_state->cols_search = state_file_rw_string(config, "cols_search", "[\"Title\",\"Artist\",\"Album\",\"Duration\"]", false);
    mympd_state->cols_browse_database = state_file_rw_string(config, "cols_browse_database", "[\"Track\",\"Title\",\"Duration\"]", false);
    mympd_state->cols_browse_playlists_detail = state_file_rw_string(config, "cols_browse_playlists_detail", "[\"Pos\",\"Title\",\"Artist\",\"Album\",\"Duration\"]", false);
    mympd_state->cols_browse_filesystem = state_file_rw_string(config, "cols_browse_filesystem", "[\"Type\",\"Title\",\"Artist\",\"Album\",\"Duration\"]", false);
    mympd_state->cols_playback = state_file_rw_string(config, "cols_playback", "[\"Artist\",\"Album\"]", false);
    mympd_state->cols_queue_last_played = state_file_rw_string(config, "cols_queue_last_played", "[\"Pos\",\"Title\",\"Artist\",\"Album\",\"LastPlayed\"]", false);
    mympd_state->feat_localplayer = state_file_rw_bool(config, "feat_localplayer", false, false);
    mympd_state->localplayer_autoplay = state_file_rw_bool(config, "localplayer_autoplay", false, false);
    mympd_state->stream_port = state_file_rw_long(config, "stream_port", 8000, false);
    mympd_state->stream_url = state_file_rw_string(config, "stream_url", "", false);
    mympd_state->bg_cover = state_file_rw_bool(config, "bg_cover", false, false);
    mympd_state->bg_color = state_file_rw_string(config, "bg_color", "#888", false);
    mympd_state->bg_css_filter = state_file_rw_string(config, "bg_css_filter", "blur(5px)", false);
    mympd_state->coverimage = state_file_rw_bool(config, "coverimage", true, false);
    mympd_state->coverimage_name = state_file_rw_string(config, "coverimage_name", "folder.jpg", false);
    mympd_state->coverimage_size = state_file_rw_long(config, "coverimage_size", 240, false);
    mympd_state->locale = state_file_rw_string(config, "locale", "default", false);
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

static long state_file_rw_long(t_config *config, const char *name, const long def_value, bool warn) {
    char *crap = NULL;
    long value = def_value;
    char def_value_str[65];
    snprintf(def_value_str, 65, "%ld", def_value);
    char *line = state_file_rw_string(config, name, def_value_str, warn);
    if (line != NULL) {
        value = strtol(line, &crap,10);
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
    int nr = 0;
    struct json_out out = JSON_OUT_BUF(buffer, MAX_SIZE);
    
    len = json_printf(&out, "{type: mympdSettings, data: {mpdhost: %Q, mpdport: %d, featSyscmds: %B, "
        "featLocalplayer: %B, streamPort: %d, streamUrl: %Q, coverimage: %B, coverimageName: %Q, coverimageSize: %d, featMixramp: %B, "
        "maxElementsPerPage: %d, notificationWeb: %B, notificationPage: %B, jukeboxMode: %d, jukeboxPlaylist: %Q, jukeboxQueueLength: %d, "
        "autoPlay: %B, bgColor: %Q, bgCover: %B, bgCssFilter: %Q, loglevel: %d, locale: %Q, localplayerAutoplay: %B", 
        config->mpdhost, 
        config->mpdport, 
        config->syscmds,
        mympd_state->feat_localplayer,
        mympd_state->stream_port,
        mympd_state->stream_url,
        mympd_state->coverimage,
        mympd_state->coverimage_name,
        mympd_state->coverimage_size,
        config->mixramp,
        config->max_elements_per_page,
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
        mympd_state->localplayer_autoplay
    );
    
    if (config->syscmds == true) {
        len += json_printf(&out, ", syscmdList: [");
        nr = 0;
        struct node *current = mympd_state->syscmd_list.list;
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

static bool mympd_api_bookmark_update(t_config *config, const long id, const char *name, const char *uri, const char *type) {
    long line_nr = 0;
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
            long lid;
            int je = json_scanf(line, read, "{id: %ld, name: %Q, uri: %Q, type: %Q}", &lid, &lname, &luri, &ltype);
            if (je == 4) {
                if (name != NULL) {
                    if (strcmp(name, lname) < 0) {
                        line_nr++;
                        struct json_out out = JSON_OUT_FILE(fo);
                        json_printf(&out, "{id: %ld, name: %Q, uri: %Q, type: %Q}\n", line_nr, name, uri, type);
                        inserted = true;
                    }
                }
                if (lid != id) {
                    line_nr++;
                    struct json_out out = JSON_OUT_FILE(fo);
                    json_printf(&out, "{id: %ld, name: %Q, uri: %Q, type: %Q}\n", line_nr, lname, luri, ltype);
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
        json_printf(&out, "{id: %ld, name: %Q, uri: %Q, type: %Q}\n", line_nr, name, uri, type);
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
    unsigned int entity_count = 0;
    unsigned int entities_returned = 0;
    struct json_out out = JSON_OUT_BUF(buffer, MAX_SIZE);
    
    snprintf(b_file, b_file_len, "%s/state/bookmarks", config->varlibdir);
    FILE *fi = fopen(b_file, "r");
    if (fi == NULL) {
        //create empty bookmarks file
        fi = fopen(b_file, "w");
        if (fi == NULL) {
            LOG_ERROR("Can't open %s for write", b_file);
            len = json_printf(&out, "{type: error, data: %Q}", "Can't open bookmarks file");
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
