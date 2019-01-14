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
static int mympd_api_put_settings(t_config *config, t_mympd_state *mympd_state, char *buffer);


//public functions
void *mympd_api_loop(void *arg_config) {
    t_config *config = (t_config *) arg_config;
    
    //read myMPD states under config.varlibdir
    t_mympd_state mympd_state;
    mympd_api_read_statefiles(config, &mympd_state);

    //push jukebox settings to mpd_client queue
    t_work_request *mpd_client_request = (t_work_request *)malloc(sizeof(t_work_request));
    mpd_client_request->conn_id = 0;
    mpd_client_request->cmd_id = MYMPD_API_SETTINGS_SET;
    mpd_client_request->length = snprintf(mpd_client_request->data, MAX_SIZE, 
        "{\"cmd\":\"MYMPD_API_SETTINGS_SET\", \"data\":{\"jukeboxMode\": %d, \"jukeboxPlaylist\": \"%s\", \"jukeboxQueueLength\": %d}}",
        mympd_state.jukeboxMode,
        mympd_state.jukeboxPlaylist,
        mympd_state.jukeboxQueueLength
    );
    tiny_queue_push(mpd_client_queue, mpd_client_request);

    //read system command files
    list_init(&mympd_state.syscmd_list);
    bool rc = mympd_api_read_syscmds(config, &mympd_state);
    if (rc == true) {
        list_sort_by_value(&mympd_state.syscmd_list, true);
    }

    while (s_signal_received == 0) {
        struct t_work_request *request = tiny_queue_shift(mympd_api_queue);
        mympd_api(config, &mympd_state, request);
    }

    list_free(&mympd_state.syscmd_list);
    return NULL;
}

//private functions
static void mympd_api(t_config *config, t_mympd_state *mympd_state, t_work_request *request) {
    //t_work_request *request = (t_work_request *) arg_request;
    size_t len = 0;
    char buffer[MAX_SIZE];
    int je;
    char *p_charbuf1;
    char p_char[4];
    LOG_VERBOSE() printf("MYMPD API request: %.*s\n", request->length, request->data);
    
    if (request->cmd_id == MYMPD_API_SYSCMD) {
        if (config->syscmds == true) {
            je = json_scanf(request->data, request->length, "{data: {cmd: %Q}}", &p_charbuf1);
            if (je == 1) {
                len = mympd_api_syscmd(config, mympd_state, buffer, p_charbuf1);
                free(p_charbuf1);
            }
        } 
        else {
            len = snprintf(buffer, MAX_SIZE, "{\"type\": \"error\", \"data\": \"System commands are disabled.\"}");
        }
    }
    else if (request->cmd_id == MYMPD_API_COLS_SAVE) {
        je = json_scanf(request->data, request->length, "{data: {table: %Q}}", &p_charbuf1);
        if (je == 1) {
            char column_list[800];
            snprintf(column_list, 800, "%.*s", request->length, request->data);
            char *cols = strchr(column_list, '[');
            int col_len = strlen(cols); 
            if (col_len > 1)
                cols[col_len - 2]  = '\0';
            if (strcmp(p_charbuf1, "colsQueueCurrent") == 0) {
                free(mympd_state->colsQueueCurrent);
                mympd_state->colsQueueCurrent = strdup(cols);
            }
            else if (strcmp(p_charbuf1, "colsSearch") == 0) {
                free(mympd_state->colsSearch);
                mympd_state->colsSearch = strdup(cols);
            }
            else if (strcmp(p_charbuf1, "colsBrowseDatabase") == 0) {
                free(mympd_state->colsBrowseDatabase);
                mympd_state->colsBrowseDatabase = strdup(cols);
            }
            else if (strcmp(p_charbuf1, "colsBrowsePlaylistsDetail") == 0) {
                free(mympd_state->colsBrowsePlaylistsDetail);
                mympd_state->colsBrowsePlaylistsDetail = strdup(cols);
            }
            else if (strcmp(p_charbuf1, "colsBrowseFilesystem") == 0) {
                free(mympd_state->colsBrowseFilesystem);
                mympd_state->colsBrowseFilesystem = strdup(cols);
            }
            else if (strcmp(p_charbuf1, "colsPlayback") == 0) {
                free(mympd_state->colsPlayback);
                mympd_state->colsPlayback = strdup(cols);
            }
            else if (strcmp(p_charbuf1, "colsQueueLastPlayed") == 0) {
                free(mympd_state->colsQueueLastPlayed);
                mympd_state->colsQueueLastPlayed = strdup(cols);
            }
            else {
                len = snprintf(buffer, MAX_SIZE, "{\"type\": \"error\", \"data\": \"Unknown table %s\"}", p_charbuf1);
                printf("MYMPD_API_COLS_SAVE: Unknown table %s\n", p_charbuf1);
            }
            if (len == 0) {
                if (state_file_write(config, p_charbuf1, cols))
                    len = snprintf(buffer, MAX_SIZE, "{\"type\": \"result\", \"data\": \"ok\"}");
            }
            free(p_charbuf1);
        }
    }
    else if (request->cmd_id == MYMPD_API_SETTINGS_SET) {
        je = json_scanf(request->data, request->length, "{data: {notificationWeb: %B}}", &mympd_state->notificationWeb);
        if (je == 1) {
            if (!state_file_write(config, "notificationWeb", (mympd_state->notificationWeb == true ? "true" : "false")))
                len = snprintf(buffer, MAX_SIZE, "{\"type\": \"error\", \"data\": \"Can't set state notificationWeb.\"}");
        }    
        je = json_scanf(request->data, request->length, "{data: {notificationPage: %B}}", &mympd_state->notificationPage);
        if (je == 1) {
            if (!state_file_write(config, "notificationPage", (mympd_state->notificationPage == true ? "true" : "false")))
                len = snprintf(buffer, MAX_SIZE, "{\"type\": \"error\", \"data\": \"Can't set state notificationPage.\"}");
        }
        je = json_scanf(request->data, request->length, "{data: {jukeboxMode: %d}}", &mympd_state->jukeboxMode);
        if (je == 1) {
            snprintf(p_char, 4, "%d", mympd_state->jukeboxMode);
            if (!state_file_write(config, "jukeboxMode", p_char))
                len = snprintf(buffer, MAX_SIZE, "{\"type\": \"error\", \"data\": \"Can't set state jukeboxMode.\"}");
        }
        je = json_scanf(request->data, request->length, "{data: {jukeboxPlaylist: %Q}}", &mympd_state->jukeboxPlaylist);
        if (je == 1) {
            if (!state_file_write(config, "jukeboxPlaylist", mympd_state->jukeboxPlaylist))
                len = snprintf(buffer, MAX_SIZE, "{\"type\": \"error\", \"data\": \"Can't set state jukeboxPlaylist.\"}");
        }
        je = json_scanf(request->data, request->length, "{data: {jukeboxQueueLength: %d}}", &mympd_state->jukeboxQueueLength);
        if (je == 1) {
           snprintf(p_char, 4, "%d", mympd_state->jukeboxQueueLength);
           if (!state_file_write(config, "jukeboxQueueLength", p_char))
               len = snprintf(buffer, MAX_SIZE, "{\"type\": \"error\", \"data\": \"Can't set state jukeboxQueueLength.\"}");
        }
        if (len == 0) {
            len = snprintf(buffer, MAX_SIZE, "{\"type\": \"result\", \"data\": \"ok\"}");
        }
        //push settings to mpd_client queue
        t_work_request *mpd_client_request = (t_work_request *)malloc(sizeof(t_work_request));
        mpd_client_request->conn_id = request->conn_id;
        mpd_client_request->cmd_id = request->cmd_id;
        mpd_client_request->length = copy_string(mpd_client_request->data, request->data, MAX_SIZE, request->length);
        tiny_queue_push(mpd_client_queue, mpd_client_request);
    }
    else if (request->cmd_id == MYMPD_API_SETTINGS_GET) {
        len = mympd_api_put_settings(config, mympd_state, buffer);
    }
    else {
        len = snprintf(buffer, MAX_SIZE, "{\"type\": \"error\", \"data\": \"Unknown cmd_id %u.\"}", request->cmd_id);
        printf("ERROR: Unknown cmd_id %u\n", request->cmd_id);    
    }

    if (len == 0) {
        len = snprintf(buffer, MAX_SIZE, "{\"type\": \"error\", \"data\": \"No response for cmd_id %u.\"}", request->cmd_id);
        printf("ERROR: No response for cmd_id %u\n", request->cmd_id);
    }
    LOG_DEBUG() fprintf(stderr, "DEBUG: Send http response to connection %lu (first 800 chars):\n%*.*s\n", request->conn_id, 0, 800, buffer);

    t_work_result *response = (t_work_result *)malloc(sizeof(t_work_result));
    response->conn_id = request->conn_id;
    response->length = copy_string(response->data, buffer, MAX_SIZE, len);
    tiny_queue_push(web_server_queue, response);

    free(request);
}

static bool mympd_api_read_syscmds(t_config *config, t_mympd_state *mympd_state) {
    DIR *dir;
    struct dirent *ent;
    char dirname[400];
    char *cmd;
    long order;

    if (config->syscmds == true) {
        snprintf(dirname, 400, "%s/syscmds", config->etcdir);
        printf("Reading syscmds: %s\n", dirname);
        if ((dir = opendir (dirname)) != NULL) {
            while ((ent = readdir(dir)) != NULL) {
                if (strncmp(ent->d_name, ".", 1) == 0)
                    continue;
                order = strtol(ent->d_name, &cmd, 10);
                if (strcmp(cmd, "") != 0)
                    list_push(&mympd_state->syscmd_list, strdup(cmd), order);
            }
            closedir(dir);
        }
        else {
            printf("ERROR: Can't read syscmds");
        }
    }
    else {
        printf("Syscmds are disabled\n");
    }
    return true;
}

static int mympd_api_syscmd(t_config *config, t_mympd_state *mympd_state, char *buffer, const char *cmd) {
    int len;
    char filename[400];
    char *line;
    char *crap;
    size_t n = 0;
    ssize_t read;
    
    const int order = list_get_value(&mympd_state->syscmd_list, cmd);
    if (order == -1) {
        printf("ERROR: Syscmd not defined: %s\n", cmd);
        len = snprintf(buffer, MAX_SIZE, "{\"type\": \"error\", \"data\": \"System command not defined\"}");
        return len;
    }
    
    snprintf(filename, 400, "%s/syscmds/%d%s", config->etcdir, order, cmd);
    FILE *fp = fopen(filename, "r");    
    if (fp == NULL) {
        len = snprintf(buffer, MAX_SIZE, "{\"type\": \"error\", \"data\": \"Can't execute cmd %s.\"}", cmd);
        printf("ERROR: Can't execute syscmd \"%s\"\n", cmd);
        return len;
    }
    read = getline(&line, &n, fp);
    fclose(fp);
    if (read > 0) {
        strtok_r(line, "\n", &crap);
        const int rc = system(line);
        if ( rc == 0) {
            len = snprintf(buffer, MAX_SIZE, "{\"type\": \"result\", \"data\": \"Executed cmd %s.\"}", cmd);
            LOG_VERBOSE() printf("Executed syscmd: \"%s\"\n", line);
        }
        else {
            len = snprintf(buffer, MAX_SIZE, "{\"type\": \"error\", \"data\": \"Executing cmd %s failed.\"}", cmd);
            printf("ERROR: Executing syscmd \"%s\" failed.\n", cmd);
        }
    } else {
        len = snprintf(buffer, MAX_SIZE, "{\"type\": \"error\", \"data\": \"Can't execute cmd %s.\"}", cmd);
        printf("ERROR: Can't execute syscmd \"%s\"\n", cmd);
    }
    CHECK_RETURN_LEN();    
}

static void mympd_api_read_statefiles(t_config *config, t_mympd_state *mympd_state) {
    char *crap;
    char value[400];

    LOG_INFO() printf("Reading states\n");
    if (state_file_read(config, "notificationWeb", value)) {
        if (strcmp(value, "true") == 0)
            mympd_state->notificationWeb = true;
        else
            mympd_state->notificationWeb = false;
    }
    else {
        mympd_state->notificationWeb = false;
        state_file_write(config, "notificationWeb", "false");
    }

    if (state_file_read(config, "notificationPage", value)) {
        if (strcmp(value, "true") == 0)
            mympd_state->notificationPage = true;
        else
            mympd_state->notificationPage = false;
    }
    else {
        mympd_state->notificationPage = true;
        state_file_write(config, "notificationPage", "true");
    }
    
    if (state_file_read(config, "jukeboxMode", value))
        mympd_state->jukeboxMode = strtol(value, &crap, 10);
    else {
        mympd_state->jukeboxMode = JUKEBOX_OFF;
        state_file_write(config, "jukeboxMode", "0");
    }

    if (state_file_read(config, "jukeboxPlaylist", value))
        mympd_state->jukeboxPlaylist = strdup(value);
    else {
        mympd_state->jukeboxPlaylist = strdup("Database");
        state_file_write(config, "jukeboxPlaylist", "Database");
    }

    if (state_file_read(config, "jukeboxQueueLength", value))
        mympd_state->jukeboxQueueLength = strtol(value, &crap, 10);
    else {
        mympd_state->jukeboxQueueLength = 1;
        state_file_write(config, "jukeboxQueueLength", "1");
    }
    
    if (state_file_read(config, "colsQueueCurrent", value))
        mympd_state->colsQueueCurrent = strdup(value);
    else {
        mympd_state->colsQueueCurrent = strdup("[\"Pos\",\"Title\",\"Artist\",\"Album\",\"Duration\"]");
        state_file_write(config, "colsQueueCurrent", mympd_state->colsQueueCurrent);
    }
    
    if (state_file_read(config, "colsSearch", value))
        mympd_state->colsSearch = strdup(value);
    else {
        mympd_state->colsSearch = strdup("[\"Title\",\"Artist\",\"Album\",\"Duration\"]");
        state_file_write(config, "colsSearch", mympd_state->colsSearch);
    }
    
    if (state_file_read(config, "colsBrowseDatabase", value))
        mympd_state->colsBrowseDatabase = strdup(value);
    else {
        mympd_state->colsBrowseDatabase = strdup("[\"Track\",\"Title\",\"Duration\"]");
        state_file_write(config, "colsBrowseDatabase", mympd_state->colsBrowseDatabase);
    }
    
    if (state_file_read(config, "colsBrowsePlaylistsDetail", value))
        mympd_state->colsBrowsePlaylistsDetail = strdup(value);
    else {
        mympd_state->colsBrowsePlaylistsDetail = strdup("[\"Pos\",\"Title\",\"Artist\",\"Album\",\"Duration\"]");
        state_file_write(config, "colsBrowsePlaylistsDetail", mympd_state->colsBrowsePlaylistsDetail);
    }
    
    if (state_file_read(config, "colsBrowseFilesystem", value))
        mympd_state->colsBrowseFilesystem = strdup(value);
    else {
        mympd_state->colsBrowseFilesystem = strdup("[\"Type\",\"Title\",\"Artist\",\"Album\",\"Duration\"]");
        state_file_write(config, "colsBrowseFilesystem", mympd_state->colsBrowseFilesystem);
    }
    
    if (state_file_read(config, "colsPlayback", value))
        mympd_state->colsPlayback = strdup(value);
    else {
        mympd_state->colsPlayback = strdup("[\"Artist\",\"Album\",\"Genre\"]");
        state_file_write(config, "colsPlayback", mympd_state->colsPlayback);
    }

    if (state_file_read(config, "colsQueueLastPlayed", value))
        mympd_state->colsQueueLastPlayed = strdup(value);
    else {
        mympd_state->colsQueueLastPlayed = strdup("[\"Pos\",\"Title\",\"Artist\",\"Album\",\"LastPlayed\"]");
        state_file_write(config, "colsQueueLastPlayed", mympd_state->colsQueueLastPlayed);
    }
}

static int mympd_api_put_settings(t_config *config, t_mympd_state *mympd_state, char *buffer) {
    size_t len;
    int nr = 0;
    struct json_out out = JSON_OUT_BUF(buffer, MAX_SIZE);
    
    len = json_printf(&out, "{type: mympdSettings, data: {mpdhost: %Q, mpdport: %d, passwort_set: %B, featSyscmds: %B, "
        "featLocalplayer: %B, streamport: %d, streamurl: %Q, featCoverimage: %B, coverimagename: %Q, coverimagesize: %d, featMixramp: %B, "
        "maxElementsPerPage: %d, notificationWeb: %B, notificationPage: %B, jukeboxMode: %d, jukeboxPlaylist: %Q, jukeboxQueueLength: %d", 
        config->mpdhost, 
        config->mpdport, 
        config->mpdpass ? "true" : "false",
        config->syscmds,
        config->localplayer,
        config->streamport,
        config->streamurl,
        config->coverimage,
        config->coverimagename,
        config->coverimagesize,
        config->mixramp,
        config->max_elements_per_page,
        mympd_state->notificationWeb,
        mympd_state->notificationPage,
        mympd_state->jukeboxMode,
        mympd_state->jukeboxPlaylist,
        mympd_state->jukeboxQueueLength
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
