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
    bool notificationWeb;
    bool notificationPage;

    //jukebox
    enum jukebox_modes jukeboxMode;
    char *jukeboxPlaylist;
    int jukeboxQueueLength;

    bool autoPlay;

    //columns
    char *colsQueueCurrent;
    char *colsSearch;
    char *colsBrowseDatabase;
    char *colsBrowsePlaylistsDetail;
    char *colsBrowseFilesystem;
    char *colsPlayback;
    char *colsQueueLastPlayed;
    
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

    //push jukebox settings to mpd_client queue
    t_work_request *mpd_client_request = (t_work_request *)malloc(sizeof(t_work_request));
    assert(mpd_client_request);
    mpd_client_request->conn_id = -1;
    mpd_client_request->cmd_id = MYMPD_API_SETTINGS_SET;
    mpd_client_request->length = snprintf(mpd_client_request->data, 1000, 
        "{\"cmd\":\"MYMPD_API_SETTINGS_SET\", \"data\":{\"jukeboxMode\": %d, \"jukeboxPlaylist\": \"%s\", \"jukeboxQueueLength\": %d, "
        "\"autoPlay\": %s}}",
        mympd_state.jukeboxMode,
        mympd_state.jukeboxPlaylist,
        mympd_state.jukeboxQueueLength,
        mympd_state.autoPlay == true ? "true" : "false"
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
    FREE_PTR(mympd_state.jukeboxPlaylist);
    FREE_PTR(mympd_state.colsQueueCurrent);
    FREE_PTR(mympd_state.colsSearch);
    FREE_PTR(mympd_state.colsBrowseDatabase);
    FREE_PTR(mympd_state.colsBrowsePlaylistsDetail);
    FREE_PTR(mympd_state.colsBrowseFilesystem);
    FREE_PTR(mympd_state.colsPlayback);
    FREE_PTR(mympd_state.colsQueueLastPlayed);
    return NULL;
}

//private functions
static void mympd_api(t_config *config, t_mympd_state *mympd_state, t_work_request *request) {
    int je;
    char *p_charbuf1 = NULL;
    char *p_charbuf2 = NULL;
    char *p_charbuf3 = NULL;
    char p_char[4];
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
                FREE_PTR(mympd_state->colsQueueCurrent);
                mympd_state->colsQueueCurrent = strdup(cols);
            }
            else if (strcmp(p_charbuf1, "colsSearch") == 0) {
                FREE_PTR(mympd_state->colsSearch);
                mympd_state->colsSearch = strdup(cols);
            }
            else if (strcmp(p_charbuf1, "colsBrowseDatabase") == 0) {
                FREE_PTR(mympd_state->colsBrowseDatabase);
                mympd_state->colsBrowseDatabase = strdup(cols);
            }
            else if (strcmp(p_charbuf1, "colsBrowsePlaylistsDetail") == 0) {
                FREE_PTR(mympd_state->colsBrowsePlaylistsDetail);
                mympd_state->colsBrowsePlaylistsDetail = strdup(cols);
            }
            else if (strcmp(p_charbuf1, "colsBrowseFilesystem") == 0) {
                FREE_PTR(mympd_state->colsBrowseFilesystem);
                mympd_state->colsBrowseFilesystem = strdup(cols);
            }
            else if (strcmp(p_charbuf1, "colsPlayback") == 0) {
                FREE_PTR(mympd_state->colsPlayback);
                mympd_state->colsPlayback = strdup(cols);
            }
            else if (strcmp(p_charbuf1, "colsQueueLastPlayed") == 0) {
                FREE_PTR(mympd_state->colsQueueLastPlayed);
                mympd_state->colsQueueLastPlayed = strdup(cols);
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
        je = json_scanf(request->data, request->length, "{data: {notificationWeb: %B}}", &mympd_state->notificationWeb);
        if (je == 1) {
            if (!state_file_write(config, "notificationWeb", (mympd_state->notificationWeb == true ? "true" : "false")))
                response->length = snprintf(response->data, MAX_SIZE, "{\"type\": \"error\", \"data\": \"Can't set state notificationWeb.\"}");
        }    
        je = json_scanf(request->data, request->length, "{data: {notificationPage: %B}}", &mympd_state->notificationPage);
        if (je == 1) {
            if (!state_file_write(config, "notificationPage", (mympd_state->notificationPage == true ? "true" : "false")))
                response->length = snprintf(response->data, MAX_SIZE, "{\"type\": \"error\", \"data\": \"Can't set state notificationPage.\"}");
        }
        je = json_scanf(request->data, request->length, "{data: {autoPlay: %B}}", &mympd_state->autoPlay);
        if (je == 1) {
            if (!state_file_write(config, "autoPlay", (mympd_state->autoPlay == true ? "true" : "false")))
                response->length = snprintf(response->data, MAX_SIZE, "{\"type\": \"error\", \"data\": \"Can't set state autoPlay.\"}");
        }
        je = json_scanf(request->data, request->length, "{data: {jukeboxMode: %d}}", &mympd_state->jukeboxMode);
        if (je == 1) {
            if (mympd_state->jukeboxMode > 2) {
                response->length = snprintf(response->data, MAX_SIZE, "{\"type\": \"error\", \"data\": \"Invalid jukeboxMode.\"}");
                LOG_ERROR("Invalid jukeboxMode");
                mympd_state->jukeboxMode = JUKEBOX_OFF;
            }
            snprintf(p_char, 4, "%d", mympd_state->jukeboxMode);
            if (!state_file_write(config, "jukeboxMode", p_char))
                response->length = snprintf(response->data, MAX_SIZE, "{\"type\": \"error\", \"data\": \"Can't set state jukeboxMode.\"}");
        }
        je = json_scanf(request->data, request->length, "{data: {jukeboxPlaylist: %Q}}", &p_charbuf1);
        if (je == 1) {
            FREE_PTR(mympd_state->jukeboxPlaylist);
            mympd_state->jukeboxPlaylist = p_charbuf1;
            p_charbuf1 = NULL;
            if (!state_file_write(config, "jukeboxPlaylist", mympd_state->jukeboxPlaylist))
                response->length = snprintf(response->data, MAX_SIZE, "{\"type\": \"error\", \"data\": \"Can't set state jukeboxPlaylist.\"}");
        }
        je = json_scanf(request->data, request->length, "{data: {jukeboxQueueLength: %d}}", &mympd_state->jukeboxQueueLength);
        if (je == 1) {
            if (mympd_state->jukeboxQueueLength > 999) {
                LOG_ERROR("jukeboxQueueLength to big, setting it to maximum value of 999");
                response->length = snprintf(response->data, MAX_SIZE, "{\"type\": \"error\", \"data\": \"jukeboxQueueLength to big, setting it to maximum value of 999\"}");
                mympd_state->jukeboxQueueLength = 999;
            }
            snprintf(p_char, 4, "%d", mympd_state->jukeboxQueueLength);
            if (!state_file_write(config, "jukeboxQueueLength", p_char))
                response->length = snprintf(response->data, MAX_SIZE, "{\"type\": \"error\", \"data\": \"Can't set state jukeboxQueueLength.\"}");
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
    mympd_state->notificationWeb = state_file_rw_bool(config, "notificationWeb", false, false);
    mympd_state->notificationPage = state_file_rw_bool(config, "notificationPage", true, false);
    mympd_state->autoPlay = state_file_rw_bool(config, "autoPlay", false, false);
    mympd_state->jukeboxMode = state_file_rw_long(config, "jukeboxMode", JUKEBOX_OFF, false);
    mympd_state->jukeboxPlaylist = state_file_rw_string(config, "jukeboxPlaylist", "Database", false);
    mympd_state->jukeboxQueueLength = state_file_rw_long(config, "jukeboxQueueLength", 1, false);
    mympd_state->colsQueueCurrent = state_file_rw_string(config, "colsQueueCurrent", "[\"Pos\",\"Title\",\"Artist\",\"Album\",\"Duration\"]", false);
    mympd_state->colsSearch = state_file_rw_string(config, "colsSearch", "[\"Title\",\"Artist\",\"Album\",\"Duration\"]", false);
    mympd_state->colsBrowseDatabase = state_file_rw_string(config, "colsBrowseDatabase", "[\"Track\",\"Title\",\"Duration\"]", false);
    mympd_state->colsBrowsePlaylistsDetail = state_file_rw_string(config, "colsBrowsePlaylistsDetail", "[\"Pos\",\"Title\",\"Artist\",\"Album\",\"Duration\"]", false);
    mympd_state->colsBrowseFilesystem = state_file_rw_string(config, "colsBrowseFilesystem", "[\"Type\",\"Title\",\"Artist\",\"Album\",\"Duration\"]", false);
    mympd_state->colsPlayback = state_file_rw_string(config, "colsPlayback", "[\"Artist\",\"Album\"]", false);
    mympd_state->colsQueueLastPlayed = state_file_rw_string(config, "colsQueueLastPlayed", "[\"Pos\",\"Title\",\"Artist\",\"Album\",\"LastPlayed\"]", false);
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
    
    len = json_printf(&out, "{type: mympdSettings, data: {mpdhost: %Q, mpdport: %d, passwort_set: %B, featSyscmds: %B, "
        "featLocalplayer: %B, streamport: %d, streamurl: %Q, coverimagename: %Q, coverimagesize: %d, featMixramp: %B, "
        "maxElementsPerPage: %d, notificationWeb: %B, notificationPage: %B, jukeboxMode: %d, jukeboxPlaylist: %Q, jukeboxQueueLength: %d, "
        "autoPlay: %B, background: %Q, backgroundFilter: %Q", 
        config->mpdhost, 
        config->mpdport, 
        config->mpdpass ? "true" : "false",
        config->syscmds,
        config->localplayer,
        config->streamport,
        config->streamurl,
        config->coverimagename,
        config->coverimagesize,
        config->mixramp,
        config->max_elements_per_page,
        mympd_state->notificationWeb,
        mympd_state->notificationPage,
        mympd_state->jukeboxMode,
        mympd_state->jukeboxPlaylist,
        mympd_state->jukeboxQueueLength,
        mympd_state->autoPlay,
        config->background,
        config->backgroundfilter
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
        mympd_state->colsQueueCurrent,
        mympd_state->colsSearch,
        mympd_state->colsBrowseDatabase,
        mympd_state->colsBrowsePlaylistsDetail,
        mympd_state->colsBrowseFilesystem,
        mympd_state->colsPlayback,
        mympd_state->colsQueueLastPlayed
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
