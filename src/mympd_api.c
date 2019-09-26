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

#include "../dist/src/sds/sds.h"
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

static void mympd_api(t_config *config, t_mympd_state *mympd_state, t_work_request *request);
static void mympd_api_push_to_mpd_client(t_mympd_state *mympd_state);
static sds mympd_api_syscmd(t_config *config, char *buffer, const char *cmd);
static void mympd_api_read_statefiles(t_config *config, t_mympd_state *mympd_state);
static sds state_file_rw_string(t_config *config, const char *name, const char *def_value, bool warn);
static bool state_file_rw_bool(t_config *config, const char *name, const bool def_value, bool warn);
static int state_file_rw_int(t_config *config, const char *name, const int def_value, bool warn);
static bool state_file_write(t_config *config, const char *name, const char *value);
static sds mympd_api_settings_put(t_config *config, t_mympd_state *mympd_state, sds buffer);
static void mympd_api_settings_reset(t_config *config, t_mympd_state *mympd_state);
static bool mympd_api_bookmark_update(t_config *config, const int id, const char *name, const char *uri, const char *type);
static sds mympd_api_bookmark_list(t_config *config, sds buffer, unsigned int offset);
static bool mympd_api_cols_save(t_config *config, t_mympd_state *mympd_state, const char *table, const char *cols);
static bool mympd_api_settings_set(t_config *config, t_mympd_state *mympd_state, struct json_token *key, struct json_token *val);
static bool mympd_api_connection_save(t_config *config, t_mympd_state *mympd_state, struct json_token *key, struct json_token *val);

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

    sds_free(mympd_state->mpd_host);
    sds_free(mympd_state->mpd_pass);
    sds_free(mympd_state->taglist);
    sds_free(mympd_state->searchtaglist);
    sds_free(mympd_state->browsetaglist);
    sds_free(mympd_state->love_channel);
    sds_free(mympd_state->love_message);
    sds_free(mympd_state->jukebox_playlist);
    sds_free(mympd_state->cols_queue_current);
    sds_free(mympd_state->cols_search);
    sds_free(mympd_state->cols_browse_database);
    sds_free(mympd_state->cols_browse_playlists_detail);
    sds_free(mympd_state->cols_browse_filesystem);
    sds_free(mympd_state->cols_playback);
    sds_free(mympd_state->cols_queue_last_played);
    sds_free(mympd_state->stream_url);
    sds_free(mympd_state->bg_color);
    sds_free(mympd_state->bg_css_filter);
    sds_free(mympd_state->coverimage_name);
    sds_free(mympd_state->locale);
    sds_free(mympd_state->music_directory);
    FREE_PTR(mympd_state);
    return NULL;
}

void mympd_api_settings_delete(t_config *config) {
    const char* state_files[]={"auto_play", "bg_color", "bg_cover", "bg_css_filter", "browsetaglist", "cols_browse_database",
        "cols_browse_filesystem", "cols_browse_playlists_detail", "cols_playback", "cols_queue_current", "cols_queue_last_played",
        "cols_search", "coverimage", "coverimage_name", "coverimage_size", "jukebox_mode", "jukebox_playlist", "jukebox_queue_length",
        "last_played", "last_played_count", "locale", "localplayer", "localplayer_autoplay", "love", "love_channel", "love_message",
        "max_elements_per_page",  "mpd_host", "mpd_pass", "mpd_port", "notification_page", "notification_web", "searchtaglist",
        "smartpls", "stickers", "stream_port", "stream_url", "taglist", "music_directory", 0};
    const char** ptr = state_files;
    sds filename = sdsempty();
    while (*ptr != 0) {
        filename = sdscatprintf(sdsempty(), "%s/state/%s", config->varlibdir, *ptr);
        unlink(filename);
        ++ptr;
    }
    sds_free(filename);
}

//private functions
static void mympd_api(t_config *config, t_mympd_state *mympd_state, t_work_request *request) {
    int je;
    char *p_charbuf1 = NULL;
    char *p_charbuf2 = NULL;
    char *p_charbuf3 = NULL;
    unsigned int uint_buf1;
    int int_buf1;
    LOG_VERBOSE("MYMPD API request (%d): %s", request->conn_id, request->data);
    
    //create response struct
    t_work_result *response = (t_work_result *)malloc(sizeof(t_work_result));
    assert(response);
    response->conn_id = request->conn_id;
    sds data = sdsempty();
    
    switch(request->cmd_id) {
        case MYMPD_API_SYSCMD:
            if (config->syscmds == true) {
                je = json_scanf(request->data, sdslen(request->data), "{data: {cmd: %Q}}", &p_charbuf1);
                if (je == 1) {
                    data = mympd_api_syscmd(config, data, p_charbuf1);
                    FREE_PTR(p_charbuf1);
                }
            } 
            else {
                data = sdscat(data, "{\"type\": \"error\", \"data\": \"System commands are disabled\"}");
            }
            break;
        case MYMPD_API_COLS_SAVE:
            je = json_scanf(request->data, sdslen(request->data), "{data: {table: %Q}}", &p_charbuf1);
            if (je == 1) {
                sdsrange(request->data, 0, -2);
                char *cols = strchr(request->data, '[');
                if (mympd_api_cols_save(config, mympd_state, p_charbuf1, cols)) {
                    data = sdscat(data, "{\"type\": \"result\", \"data\": \"ok\"}");
                }
                else {
                    sds tablename = sdscatrepr(sdsempty(), p_charbuf1, strlen(p_charbuf1));
                    data = sdscatprintf(data, "{\"type\": \"error\", \"data\": \"Unknown table %%{table}\", \"values\": {\"table\": \"%s\"}}", tablename);
                    LOG_ERROR("MYMPD_API_COLS_SAVE: Unknown table %s", tablename);
                    sds_free(tablename);
                }
                FREE_PTR(p_charbuf1);
            }
            break;
        case MYMPD_API_SETTINGS_RESET:
            mympd_api_settings_reset(config, mympd_state);
            data = sdscat(data, "{\"type\": \"result\", \"data\": \"ok\"}");
            break;
        case MYMPD_API_SETTINGS_SET: {
            void *h = NULL;
            struct json_token key;
            struct json_token val;
            bool rc = true;
            while ((h = json_next_key(request->data, sdslen(request->data), h, ".data", &key, &val)) != NULL) {
                rc = mympd_api_settings_set(config, mympd_state, &key, &val);
                if (rc == false) {
                    break;
                }
            }
            if (rc == true) {
                //forward request to mpd_client queue            
                t_work_request *mpd_client_request = (t_work_request *)malloc(sizeof(t_work_request));
                assert(mpd_client_request);
                mpd_client_request->conn_id = -1;
                mpd_client_request->cmd_id = request->cmd_id;
                mpd_client_request->data = sdsdup(request->data);
                tiny_queue_push(mpd_client_queue, mpd_client_request);
                data = sdscat(data, "{\"type\": \"result\", \"data\": \"ok\"}");
            }
            else {
                sds settingname = sdscatrepr(sdsempty(), val.ptr, val.len);
                data = sdscatprintf(data, "{\"type\": \"error\", \"data\": \"Can't save setting %%{setting}\", \"values\": {\"setting\": \"%s\"}}", settingname);
                sds_free(settingname);
            }
            break;
        }
        case MYMPD_API_SETTINGS_GET:
            data = mympd_api_settings_put(config, mympd_state, data);
            break;
        case MYMPD_API_CONNECTION_SAVE: {
            void *h = NULL;
            struct json_token key;
            struct json_token val;
            bool rc = true;
            while ((h = json_next_key(request->data, sdslen(request->data), h, ".data", &key, &val)) != NULL) {
                rc = mympd_api_connection_save(config, mympd_state, &key, &val);
                if (rc == false) {
                    break;
                }
            }
            if (rc == true) {
                //push settings to mpd_client queue
                mympd_api_push_to_mpd_client(mympd_state);
                data = sdscat(data, "{\"type\": \"result\", \"data\": \"ok\"}");
            }
            else {
                sds settingname = sdscatrepr(sdsempty(), val.ptr, val.len);
                data = sdscatprintf(data, "{\"type\": \"error\", \"data\": \"Can't save setting %%{setting}\", \"values\": {\"setting\": \"%s\"}}", settingname);
                LOG_ERROR("MYMPD_API_CONNECTION_SAVE: Can't save setting %s", settingname);
                sds_free(settingname);
            }
            break;
        }
        case MYMPD_API_BOOKMARK_SAVE:
            je = json_scanf(request->data, sdslen(request->data), "{data: {id: %d, name: %Q, uri: %Q, type: %Q}}", &int_buf1, &p_charbuf1, &p_charbuf2, &p_charbuf3);
            if (je == 4) {
                if (mympd_api_bookmark_update(config, int_buf1, p_charbuf1, p_charbuf2, p_charbuf3)) {
                    data = sdscat(data, "{\"type\": \"result\", \"data\": \"ok\"}");
                }
                else {
                    data = sdscat(data, "{\"type\": \"error\", \"data\": \"Saving bookmark failed\"}");
                }
                FREE_PTR(p_charbuf1);
                FREE_PTR(p_charbuf2);
                FREE_PTR(p_charbuf3);
            }
            break;
        case MYMPD_API_BOOKMARK_RM:
            je = json_scanf(request->data, sdslen(request->data), "{data: {id: %d}}", &int_buf1);
            if (je == 1) {
                if (mympd_api_bookmark_update(config, int_buf1, NULL, NULL, NULL)) {
                    data = sdscat(data, "{\"type\": \"result\", \"data\": \"ok\"}");
                }
                else {
                    data = sdscat(data, "{\"type\": \"error\", \"data\": \"Deleting bookmark failed\"}");
                }
            }
            break;
        case MYMPD_API_BOOKMARK_LIST:
            je = json_scanf(request->data, sdslen(request->data), "{data: {offset: %u}}", &uint_buf1);
            if (je == 1) {
                data = mympd_api_bookmark_list(config, data, uint_buf1);
            }
            break;
        default:
            data = sdscat(data, "{\"type\": \"error\", \"data\": \"Unknown request\"}");
            LOG_ERROR("Unknown cmd_id %u", request->cmd_id);
    }

    if (sdslen(data) == 0) {
        data = sdscatprintf(data, "{\"type\": \"error\", \"data\": \"No response for method %%{method}\", \"values\": {\"method\": %s}}", request->method);
        LOG_ERROR("No response for cmd_id %u", request->cmd_id);
    }
    response->data = data;
    LOG_DEBUG("Push response to queue for connection %lu: %s", request->conn_id, response->data);
    tiny_queue_push(web_server_queue, response);
    sds_free(request->data);
    sds_free(request->method);
    FREE_PTR(request);
}

static void mympd_api_push_to_mpd_client(t_mympd_state *mympd_state) {
    t_work_request *mpd_client_request = (t_work_request *)malloc(sizeof(t_work_request));
    assert(mpd_client_request);
    mpd_client_request->conn_id = -1;
    mpd_client_request->cmd_id = MYMPD_API_SETTINGS_SET;
    sds data = sdsempty();
    
    data = sdscat(data, "{\"cmd\":\"MYMPD_API_SETTINGS_SET\",\"data\":{");
    data = tojson_long(data, "jukeboxMode", mympd_state->jukebox_mode, true);
    data = tojson_char(data, "jukeboxPlaylist", mympd_state->jukebox_playlist, true);
    data = tojson_long(data, "jukeboxQueueLength", mympd_state->jukebox_queue_length, true);
    data = tojson_bool(data, "autoPlay", mympd_state->auto_play, true);
    data = tojson_bool(data, "coverimage,", mympd_state->coverimage, true);
    data = tojson_char(data, "coverimageName", mympd_state->coverimage_name, true);
    data = tojson_bool(data, "love", mympd_state->love, true);
    data = tojson_char(data, "loveChannel", mympd_state->love_channel, true);
    data = tojson_char(data, "loveMessage", mympd_state->love_message, true);
    data = tojson_char(data, "taglist", mympd_state->taglist, true);
    data = tojson_char(data, "searchtaglist", mympd_state->searchtaglist, true);
    data = tojson_char(data, "browsetaglist", mympd_state->browsetaglist, true);
    data = tojson_bool(data, "stickers", mympd_state->stickers, true);
    data = tojson_bool(data, "smartpls", mympd_state->smartpls, true);
    data = tojson_char(data, "mpdHost", mympd_state->mpd_host, true);
    data = tojson_char(data, "mpdPass", mympd_state->mpd_pass, true);
    data = tojson_long(data, "mpdPort", mympd_state->mpd_port, true);
    data = tojson_long(data, "lastPlayedCount", mympd_state->last_played_count, true);
    data = tojson_long(data, "maxElementsPerPage", mympd_state->max_elements_per_page, true);
    data = tojson_char(data, "musicDirectory", mympd_state->music_directory, false);
    data = sdscat(data, "}}");

    mpd_client_request->data = data;    
    tiny_queue_push(mpd_client_queue, mpd_client_request);
}

static bool mympd_api_connection_save(t_config *config, t_mympd_state *mympd_state, struct json_token *key, struct json_token *val) {
    char *crap;
    sds settingname = sdsempty();
    sds settingvalue = sdscatlen(sdsempty(), val->ptr, val->len);
    if (strncmp(key->ptr, "mpdPass", key->len) == 0) {
        if (strcmp(key, "dontsetpassword") != 0) {
            mympd_state->mpd_pass = sdscatlen(sdsempty(), settingvalue, sdslen(settingvalue));
            settingname = sdscat(sdsempty(), "mpd_pass");
        }
        else {
            sds_free(settingname);
            sds_free(settingvalue);
            return true;
        }
    }
    else if (strncmp(key->ptr, "mpdHost", key->len) == 0) {
        mympd_state->mpd_host = sdscatlen(sdsempty(), settingvalue, sdslen(settingvalue));
        settingname = sdscat(sdsempty(), "mpd_host");
    }
    else if (strncmp(key->ptr, "mpdPort", key->len) == 0) {
        mympd_state->mpd_port = strtoimax(settingvalue, &crap, 10);
        settingname = sdscat(sdsempty(), "mpd_port");
    }
    else if (strncmp(key->ptr, "musicDirectory", key->len) == 0) {
        mympd_state->mpd_host = sdscatlen(sdsempty(), settingvalue, sdslen(settingvalue));
        settingname = sdscat(sdsempty(), "music_directory");
    }
    else {
        sds_free(settingname);
        sds_free(settingvalue);
        return true;
    }

    bool rc = state_file_write(config, settingname, settingvalue);
    sds_free(settingname);
    sds_free(settingvalue);
    return rc;
}

static sds mympd_api_syscmd(t_config *config, char *buffer, const char *cmd) {
    sds cmdline = list_get_extra(&config->syscmd_list, cmd);
    if (cmdline == NULL) {
        LOG_ERROR("Syscmd not defined: %s", cmd);
        buffer = sdscat(buffer, "{\"type\": \"error\", \"data\": \"System command not defined\"}");
        return buffer;
    }

    const int rc = system(cmdline);
    if ( rc == 0) {
        buffer = sdscatprintf(buffer, "{\"type\": \"result\", \"data\": \"Successfully execute cmd %%{cmd}\", \"values\":{\"cmd\": \"%s\"}}", cmd);
        LOG_VERBOSE("Executed syscmd: \"%s\"", cmdline);
    }
    else {
        buffer = sdscatprintf(buffer, "{\"type\": \"error\", \"data\": \"Failed to execute cmd %%{cmd}\", \"values\":{\"cmd\": \"%s\"}}", cmd);
        LOG_ERROR("Executing syscmd \"%s\" failed", cmdline);
    }
    return buffer;
}

static bool mympd_api_cols_save(t_config *config, t_mympd_state *mympd_state, const char *table, const char *cols) {
    if (strcmp(table, "colsQueueCurrent") == 0) {
        mympd_state->cols_queue_current = sdscat(sdsempty(), cols);
    }
    else if (strcmp(table, "colsSearch") == 0) {
        mympd_state->cols_search = sdscat(sdsempty(), cols);
    }
    else if (strcmp(table, "colsBrowseDatabase") == 0) {
        mympd_state->cols_browse_database = sdscat(sdsempty(), cols);
    }
    else if (strcmp(table, "colsBrowsePlaylistsDetail") == 0) {
        mympd_state->cols_browse_playlists_detail = sdscat(sdsempty(), cols);
    }
    else if (strcmp(table, "colsBrowseFilesystem") == 0) {
        mympd_state->cols_browse_filesystem = sdscat(sdsempty(), cols);
    }
    else if (strcmp(table, "colsPlayback") == 0) {
        mympd_state->cols_playback = sdscat(sdsempty(), cols);
    }
    else if (strcmp(table, "colsQueueLastPlayed") == 0) {
        mympd_state->cols_queue_last_played = sdscat(sdsempty(), cols);
    }
    else {
        return false;
    }
    
    if (!state_file_write(config, table, cols)) {
        return false;
    }
    
    return true;
}

static bool mympd_api_settings_set(t_config *config, t_mympd_state *mympd_state, struct json_token *key, struct json_token *val) {
    sds settingname = sdsempty();
    sds settingvalue = sdscatlen(sdsempty(), key->ptr, key->len);
    char *crap;
    
    if (strncmp(key->ptr, "notificationWeb", key->len) == 0) {
        mympd_state->notification_web = val->type == JSON_TYPE_TRUE ? true : false;
        settingname = sdscat(sdsempty(), "notification_web");
    }
    else if (strncmp(key->ptr, "notificationPage", key->len) == 0) {
        mympd_state->notification_page = val->type == JSON_TYPE_TRUE ? true : false;
        settingname = sdscat(sdsempty(), "notification_page");
    }
    else if (strncmp(key->ptr, "autoPlay", key->len) == 0) {
        mympd_state->auto_play = val->type == JSON_TYPE_TRUE ? true : false;
        settingname = sdscat(sdsempty(), "auto_play");
    }
    else if (strncmp(key->ptr, "localplayerAutoplay", key->len) == 0) {
        mympd_state->localplayer_autoplay = val->type == JSON_TYPE_TRUE ? true : false;
        settingname = sdscat(sdsempty(), "localplayer_autoplay");
    }
    else if (strncmp(key->ptr, "coverimage", key->len) == 0) {
        mympd_state->coverimage = val->type == JSON_TYPE_TRUE ? true : false;
        settingname = sdscat(sdsempty(), "coverimage");
    }
    else if (strncmp(key->ptr, "coverimageName", key->len) == 0) {
        if (validate_string(settingvalue) && sdslen(settingvalue) > 0) {
            mympd_state->coverimage_name = sdscatlen(sdsempty(), settingvalue, sdslen(settingvalue));
            settingname = sdscat(sdsempty(), "coverimage_name");
        }
        else {
            sds_free(settingname);
            sds_free(settingvalue);
            return false;
        }
    }
    else if (strncmp(key->ptr, "coverimageSize", key->len) == 0) {
        mympd_state->coverimage_size = strtoimax(settingvalue, &crap, 10);
        settingname = sdscat(sdsempty(), "coverimage_size");
    }
    else if (strncmp(key->ptr, "featLocalplayer", key->len) == 0) {
        mympd_state->localplayer = val->type == JSON_TYPE_TRUE ? true : false;
        settingname = sdscat(sdsempty(), "localplayer");
    }
    else if (strncmp(key->ptr, "streamPort", key->len) == 0) {
        mympd_state->stream_port = strtoimax(settingvalue, &crap, 10);
        settingname = sdscat(sdsempty(), "stream_port");
    }
    else if (strncmp(key->ptr, "streamUrl", key->len) == 0) {
        mympd_state->stream_url = sdscatlen(sdsempty(), settingvalue, sdslen(settingvalue));
        settingname = sdscat(sdsempty(), "stream_url");
    }
    else if (strncmp(key->ptr, "locale", key->len) == 0) {
        mympd_state->locale = sdscatlen(sdsempty(), settingvalue, sdslen(settingvalue));
        settingname = sdscat(sdsempty(), "locale");
    }
    else if (strncmp(key->ptr, "bgCover", key->len) == 0) {
        mympd_state->bg_cover = val->type == JSON_TYPE_TRUE ? true : false;
        settingname = sdscat(sdsempty(), "bg_cover");
    }
    else if (strncmp(key->ptr, "bgColor", key->len) == 0) {
        mympd_state->bg_color = sdscatlen(sdsempty(), settingvalue, sdslen(settingvalue));
        settingname = sdscat(sdsempty(), "bg_color");
    }
    else if (strncmp(key->ptr, "bgCssFilter", key->len) == 0) {
        mympd_state->bg_css_filter = sdscatlen(sdsempty(), settingvalue, sdslen(settingvalue));
        settingname = sdscat(sdsempty(), "bg_css_filter");
    }
    else if (strncmp(key->ptr, "jukeboxMode", key->len) == 0) {
        int jukebox_mode = strtoimax(settingvalue, &crap, 10);
        if (jukebox_mode < 0 || jukebox_mode > 2) {
            sds_free(settingname);
            sds_free(settingvalue);
            return false;
        }
        mympd_state->jukebox_mode = jukebox_mode;
        settingname = sdscat(sdsempty(), "jukebox_mode");
    }
    else if (strncmp(key->ptr, "jukeboxPlaylist", key->len) == 0) {
        mympd_state->jukebox_playlist = sdscatlen(sdsempty(), settingvalue, sdslen(settingvalue));
        settingname = sdscat(sdsempty(), "jukebox_playlist");
    }
    else if (strncmp(key->ptr, "jukeboxQueueLength", key->len) == 0) {
        int jukebox_queue_length = strtoimax(settingvalue, &crap, 10);
        if (jukebox_queue_length <= 0 || jukebox_queue_length > 999) {
            sds_free(settingname);
            sds_free(settingvalue);
            return false;
        }
        mympd_state->jukebox_queue_length = jukebox_queue_length;
        settingname = sdscat(sdsempty(), "jukebox_queue_length");
    }
    else if (strncmp(key->ptr, "stickers", key->len) == 0) {
        mympd_state->stickers = val->type == JSON_TYPE_TRUE ? true : false;
        settingname = sdscat(sdsempty(), "stickers");
    }
    else if (strncmp(key->ptr, "lastPlayedCount", key->len) == 0) {
        int last_played_count = strtoimax(settingvalue, &crap, 10);
        if (last_played_count <= 0) {
            sds_free(settingname);
            sds_free(settingvalue);
            return false;
        }
        mympd_state->last_played_count = last_played_count;
        settingname = sdscat(sdsempty(), "last_played_count");
    }
    else if (strncmp(key->ptr, "taglist", key->len) == 0) {
        mympd_state->taglist = sdscatlen(sdsempty(), settingvalue, sdslen(settingvalue));
        settingname = sdscat(sdsempty(), "taglist");
    }
    else if (strncmp(key->ptr, "searchtaglist", key->len) == 0) {
        mympd_state->searchtaglist = sdscatlen(sdsempty(), settingvalue, sdslen(settingvalue));
        settingname = sdscat(sdsempty(), "searchtaglist");
    }
    else if (strncmp(key->ptr, "browsetaglist", key->len) == 0) {
        mympd_state->browsetaglist = sdscatlen(sdsempty(), settingvalue, sdslen(settingvalue));
        settingname = sdscat(sdsempty(), "browsetaglist");
    }
    else if (strncmp(key->ptr, "smartpls", key->len) == 0) {
        mympd_state->smartpls = val->type == JSON_TYPE_TRUE ? true : false;
        settingname = sdscat(sdsempty(), "smartpls");
    }
    else if (strncmp(key->ptr, "maxElementsPerPage", key->len) == 0) {
        int max_elements_per_page = strtoimax(settingvalue, &crap, 10);
        if (max_elements_per_page <= 0 || max_elements_per_page > 999) {
            sds_free(settingname);
            sds_free(settingvalue);
            return false;
        }
        mympd_state->max_elements_per_page = max_elements_per_page;
        settingname = sdscat(sdsempty(), "max_elements_per_page");
    }
    else if (strncmp(key->ptr, "love", key->len) == 0) {
        mympd_state->love = val->type == JSON_TYPE_TRUE ? true : false;
        settingname = sdscat(sdsempty(), "love");
    }
    else if (strcmp(key->ptr, "loveChannel", key->len) == 0) {
        mympd_state->love_channel = sdscatlen(sdsempty(), settingvalue, sdslen(settingvalue));
        settingname = sdscat(sdsempty(), "love_channel");
    }
    else if (strncmp(key->ptr, "loveMessage", key->len) == 0) {
        mympd_state->love_message = sdscatlen(sdsempty(), settingvalue, sdslen(settingvalue));
        settingname = sdscat(sdsempty(), "love_message");
    }
    else {
        sds_free(settingname);
        sds_free(settingvalue);
        return true;
    }
    bool rc = state_file_write(config, settingname, settingvalue);
    sds_free(settingname);
    sds_free(settingvalue);
    return rc;
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

static sds state_file_rw_string(t_config *config, const char *name, const char *def_value, bool warn) {
    char *line = NULL;
    size_t n = 0;
    ssize_t read;
    
    sds result = sdsempty();
    
    if (!validate_string(name)) {
        return result;
    }
    
    sds cfg_file = sdscatprintf(sdsempty(), "%s/state/%s", config->varlibdir, name);
    FILE *fp = fopen(cfg_file, "r");
    sds_free(cfg_file);
    if (fp == NULL) {
        if (warn == true) {
            LOG_WARN("Can't open %s", cfg_file);
        }
        state_file_write(config, name, def_value);
        result = sdscat(sdsempty(), def_value);
        return result;
    }
    read = getline(&line, &n, fp);
    if (read > 0) {
        LOG_DEBUG("State %s: %s", name, line);
    }
    fclose(fp);
    if (read > 0) {
        result = sdscat(sdsempty(), line);
        FREE_PTR(line);
        return result;
    }
    else {
        FREE_PTR(line);
        result = sdscat(sdsempty(), def_value);
        return result;
    }
}

static bool state_file_rw_bool(t_config *config, const char *name, const bool def_value, bool warn) {
    bool value = def_value;
    sds line = state_file_rw_string(config, name, def_value == true ? "true" : "false", warn);
    if (sdslen(line) > 0) {
        value = strcmp(line, "true") == 0 ? true : false;
        sds_free(line);
    }
    return value;
}

static int state_file_rw_int(t_config *config, const char *name, const int def_value, bool warn) {
    char *crap = NULL;
    int value = def_value;
    sds def_value_str = sdsfromlonglong(def_value);
    sds line = state_file_rw_string(config, name, def_value_str, warn);
    if (sdslen(line) > 0) {
        value = strtoimax(line, &crap, 10);
        sds_free(line);
    }
    return value;
}

static bool state_file_write(t_config *config, const char *name, const char *value) {
    if (!validate_string(name)) {
        return false;
    }
    sds tmp_file = sdscatprintf(sdsempty(), "%s/state/%s.XXXXXX", config->varlibdir, name);
    int fd;
    if ((fd = mkstemp(tmp_file)) < 0 ) {
        LOG_ERROR("Can't open %s for write", tmp_file);
        sds_free(tmp_file);
        return false;
    }
    FILE *fp = fdopen(fd, "w");
    fprintf(fp, "%s", value);
    fclose(fp);
    sds cfg_file = sdscatprintf(sdsempty(), "%s/state/%s", config->varlibdir, name);
    if (rename(tmp_file, cfg_file) == -1) {
        LOG_ERROR("Renaming file from %s to %s failed", tmp_file, cfg_file);
        sds_free(tmp_file);
        sds_free(cfg_file);
        return false;
    }
    sds_free(tmp_file);
    sds_free(cfg_file);
    return true;
}

static sds mympd_api_settings_put(t_config *config, t_mympd_state *mympd_state, sds buffer) {
    buffer = sdscat(buffer, "{\"type\":\"mympdSettings\",\"data\":{");
    buffer = tojson_char(buffer, "mpdHost", mympd_state->mpd_host, true);
    buffer = tojson_long(buffer, "mpdPort", mympd_state->mpd_port, true);
    buffer = tojson_char(buffer, "mpdPass", "dontsetpassword", true);
    buffer = tojson_bool(buffer, "featSyscmds", config->syscmds, true);
    buffer = tojson_bool(buffer, "featCacert", (config->custom_cert == false && config->ssl == true ? true : false), true);
    buffer = tojson_bool(buffer, "featLocalplayer", mympd_state->localplayer, true);
    buffer = tojson_long(buffer, "streamPort", mympd_state->stream_port, true);
    buffer = tojson_char(buffer, "streamUrl", mympd_state->stream_url, true);
    buffer = tojson_bool(buffer, "coverimage", mympd_state->coverimage, true);
    buffer = tojson_char(buffer, "coverimageName", mympd_state->coverimage_name, true);
    buffer = tojson_long(buffer, "coverimageSize", mympd_state->coverimage_size, true);
    buffer = tojson_bool(buffer, "featMixramp", config->mixramp, true);
    buffer = tojson_long(buffer, "maxElementsPerPage", mympd_state->max_elements_per_page, true);
    buffer = tojson_bool(buffer, "notificationWeb", mympd_state->notification_web, true);
    buffer = tojson_bool(buffer, "notificationPage", mympd_state->notification_page, true);
    buffer = tojson_long(buffer, "jukeboxMode", mympd_state->jukebox_mode, true);
    buffer = tojson_char(buffer, "jukeboxPlaylist", mympd_state->jukebox_playlist, true);
    buffer = tojson_long(buffer, "jukeboxQueueLength", mympd_state->jukebox_queue_length, true);
    buffer = tojson_bool(buffer, "autoPlay", mympd_state->auto_play, true);
    buffer = tojson_char(buffer, "bgColor", mympd_state->bg_color, true);
    buffer = tojson_bool(buffer, "bgCover", mympd_state->bg_cover, true);
    buffer = tojson_char(buffer, "bgCssFilter", mympd_state->bg_css_filter, true);
    buffer = tojson_long(buffer, "loglevel", loglevel, true);
    buffer = tojson_char(buffer, "locale", mympd_state->locale, true);
    buffer = tojson_bool(buffer, "localplayerAutoplay", mympd_state->localplayer_autoplay, true);
    buffer = tojson_bool(buffer, "stickers", mympd_state->stickers, true);
    buffer = tojson_bool(buffer, "smartpls", mympd_state->smartpls, true);
    buffer = tojson_long(buffer, "lastPlayedCount", mympd_state->last_played_count, true);
    buffer = tojson_bool(buffer, "love", mympd_state->love, true);
    buffer = tojson_char(buffer, "loveChannel", mympd_state->love_channel, true);
    buffer = tojson_char(buffer, "loveMessage", mympd_state->love_message, true);
    buffer = tojson_char(buffer, "musicDirectory", mympd_state->music_directory, true);
    buffer = sdscatprintf(buffer, "\"colsQueueCurrent\":%s,", mympd_state->cols_queue_current);
    buffer = sdscatprintf(buffer, "\"colsSearch\":%s,", mympd_state->cols_search);
    buffer = sdscatprintf(buffer, "\"colsBrowseDatabase\":%s,", mympd_state->cols_browse_database);
    buffer = sdscatprintf(buffer, "\"colsBrowsePlaylistsDetail\":%s,", mympd_state->cols_browse_playlists_detail);
    buffer = sdscatprintf(buffer, "\"colsBrowseFilesystem\":%s,", mympd_state->cols_browse_filesystem);
    buffer = sdscatprintf(buffer, "\"colsPlayback\":%s,", mympd_state->cols_playback);
    buffer = sdscatprintf(buffer, "\"colsQueueLastPlayed\":%s", mympd_state->cols_queue_last_played);

    if (config->syscmds == true) {
        buffer = sdscat(buffer, ",\"syscmdList\":[");
        int nr = 0;
        struct node *current = config->syscmd_list.list;
        while (current != NULL) {
            if (nr++) {
                buffer = sdscat(buffer, ",");
            }
            buffer = sdscatrepr(buffer, current->data, strlen(current->data));
            current = current->next;
        }
        buffer = sdscat(buffer, "]");
    }

    buffer = sdscat(buffer, "}}");
    return buffer;
}

static bool mympd_api_bookmark_update(t_config *config, const int id, const char *name, const char *uri, const char *type) {
    int line_nr = 0;
    char *line = NULL;
    size_t n = 0;
    ssize_t read;
    bool inserted = false;
    int fd;
    sds tmp_file = sdscatprintf(sdsempty(), "%s/state/bookmarks.XXXXXX", config->varlibdir);
    
    if ((fd = mkstemp(tmp_file)) < 0 ) {
        LOG_ERROR("Can't open %s for write", tmp_file);
        sds_free(tmp_file);
        return false;
    }
    FILE *fo = fdopen(fd, "w");
    
    sds b_file = sdscatprintf(sdsempty(), "%s/state/bookmarks", config->varlibdir);
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
        sds_free(tmp_file);
        sds_free(b_file);
        return false;
    }
    sds_free(tmp_file);
    sds_free(b_file);
    return true;
}

static sds mympd_api_bookmark_list(t_config *config, sds buffer, unsigned int offset) {
    size_t len = 0;
    char *line = NULL;
    char *crap = NULL;
    size_t n = 0;
    ssize_t read;
    unsigned entity_count = 0;
    unsigned entities_returned = 0;
    
    sds b_file = sdscatprintf(sdsempty(), "%s/state/bookmarks", config->varlibdir);
    FILE *fi = fopen(b_file, "r");

    if (fi == NULL) {
        //create empty bookmarks file
        fi = fopen(b_file, "w");
        if (fi == NULL) {
            LOG_ERROR("Can't open %s for write", b_file);
            buffer = sdscat(sdsempty(), "{\"type\":\"error\",\"data\":\"Failed to open bookmarks file\"}");
        }
        else {
            fclose(fi);
            buffer = sdscat(sdsempty(), "{\"type\":\"bookmark\",\"data\":[],\"totalEntities\":0,\"offset\":0,\"returnedEntities\":0}");
        }
    } else {
        buffer = sdscat(buffer, "{\"type\":\"bookmark\",\"data\":[");
        while ((read = getline(&line, &n, fi)) > 0 && len < MAX_LIST_SIZE) {
            entity_count++;
            if (entity_count > offset && entity_count <= offset + config->max_elements_per_page) {
                if (entities_returned++) {
                    buffer = sdscat(buffer, ",");
                }
                strtok_r(line, "\n", &crap);
                buffer = sdscat(buffer, line);
            }
        }
        FREE_PTR(line);
        fclose(fi);
        buffer = sdscatprintf(buffer, "],\"totalEntities\":%d,\"offset\":%d,\"returnedEntities\":%d}",
            entity_count,
            offset,
            entities_returned
        );
    }
    sds_free(b_file);
    return buffer;
}
