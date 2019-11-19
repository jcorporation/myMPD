/*
 SPDX-License-Identifier: GPL-2.0-or-later
 myMPD (c) 2018-2019 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
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
#include "sds_extras.h"
#include "../dist/src/frozen/frozen.h"
#include "api.h"
#include "utility.h"
#include "log.h"
#include "list.h"
#include "tiny_queue.h"
#include "config_defs.h"
#include "global.h"
#include "mpd_client.h"
#include "maintenance.h"
#include "mympd_api/mympd_api_utility.h"
#include "mympd_api/mympd_api_settings.h"
#include "mympd_api/mympd_api_syscmds.h"
#include "mympd_api/mympd_api_bookmarks.h"
#include "mympd_api.h"

//private definitions
static void mympd_api(t_config *config, t_mympd_state *mympd_state, t_work_request *request);

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

    free_mympd_state(mympd_state);
    return NULL;
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
                je = json_scanf(request->data, sdslen(request->data), "{params: {cmd: %Q}}", &p_charbuf1);
                if (je == 1) {
                    data = mympd_api_syscmd(config, data, request->method, request->id, p_charbuf1);
                }
            } 
            else {
                data = jsonrpc_respond_message(data, request->method, request->id, "System commands are disabled", true);
            }
            break;
        case MYMPD_API_COLS_SAVE: {
            sds cols = sdsnewlen("[", 1);
            je = json_scanf(request->data, sdslen(request->data), "{params: {table: %Q}}", &p_charbuf1);
            if (je == 1) {
                cols = json_to_cols(cols, request->data, sdslen(request->data));
                cols = sdscatlen(cols, "]", 1);
                if (mympd_api_cols_save(config, mympd_state, p_charbuf1, cols)) {
                    data = jsonrpc_respond_ok(data, request->method, request->id);
                }
                else {
                    data = jsonrpc_start_phrase(data, request->method, request->id, "Unknown table %{table}", true);
                    data = tojson_char(data, "table", p_charbuf1, false);
                    data = jsonrpc_end_phrase(data);
                    LOG_ERROR("MYMPD_API_COLS_SAVE: Unknown table %s", p_charbuf1);
                }            
            }
            sdsfree(cols);
            break;
        }
        case MYMPD_API_SETTINGS_RESET:
            //ToDo: error checking
            mympd_api_settings_reset(config, mympd_state);
            data = jsonrpc_respond_ok(data, request->method, request->id);
            break;
        case MYMPD_API_SETTINGS_SET: {
            void *h = NULL;
            struct json_token key;
            struct json_token val;
            bool rc = true;
            while ((h = json_next_key(request->data, sdslen(request->data), h, ".params", &key, &val)) != NULL) {
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
                mpd_client_request->id = request->id;
                mpd_client_request->method = sdsdup(request->method);
                mpd_client_request->data = sdsdup(request->data);
                tiny_queue_push(mpd_client_queue, mpd_client_request);
                data = jsonrpc_respond_ok(data, request->method, request->id);
            }
            else {
                data = jsonrpc_start_phrase(data, request->method, request->id, "Can't save setting %{setting}", true);
                data = tojson_char_len(data, "setting", key.ptr, key.len, false);
                data = jsonrpc_end_phrase(data);
            }
            break;
        }
        case MYMPD_API_SETTINGS_GET:
            data = mympd_api_settings_put(config, mympd_state, data, request->method, request->id);
            break;
        case MYMPD_API_CONNECTION_SAVE: {
            void *h = NULL;
            struct json_token key;
            struct json_token val;
            bool rc = true;
            while ((h = json_next_key(request->data, sdslen(request->data), h, ".params", &key, &val)) != NULL) {
                rc = mympd_api_connection_save(config, mympd_state, &key, &val);
                if (rc == false) {
                    break;
                }
            }
            if (rc == true) {
                //push settings to mpd_client queue
                mympd_api_push_to_mpd_client(mympd_state);
                data = jsonrpc_respond_ok(data, request->method, request->id);
            }
            else {
                data = jsonrpc_start_phrase(data, request->method, request->id, "Can't save setting %{setting}", true);
                data = tojson_char_len(data, "setting", val.ptr, val.len, false);
                data = jsonrpc_end_phrase(data);
            }
            break;
        }
        case MYMPD_API_BOOKMARK_SAVE:
            je = json_scanf(request->data, sdslen(request->data), "{params: {id: %d, name: %Q, uri: %Q, type: %Q}}", &int_buf1, &p_charbuf1, &p_charbuf2, &p_charbuf3);
            if (je == 4) {
                if (mympd_api_bookmark_update(config, int_buf1, p_charbuf1, p_charbuf2, p_charbuf3)) {
                    data = jsonrpc_respond_ok(data, request->method, request->id);
                }
                else {
                    data = jsonrpc_respond_message(data, request->method, request->id, "Saving bookmark failed", true);
                }
            }
            break;
        case MYMPD_API_BOOKMARK_RM:
            je = json_scanf(request->data, sdslen(request->data), "{params: {id: %d}}", &int_buf1);
            if (je == 1) {
                if (mympd_api_bookmark_update(config, int_buf1, NULL, NULL, NULL)) {
                    data = jsonrpc_respond_ok(data, request->method, request->id);
                }
                else {
                    data = jsonrpc_respond_message(data, request->method, request->id, "Deleting bookmark failed", true);
                }
            }
            break;
        case MYMPD_API_BOOKMARK_CLEAR:
            if (mympd_api_bookmark_clear(config)) {
                data = jsonrpc_respond_ok(data, request->method, request->id);
            }
            else {
                data = jsonrpc_respond_message(data, request->method, request->id, "Clearing bookmarks failed", true);
            }
            break;
        case MYMPD_API_BOOKMARK_LIST:
            je = json_scanf(request->data, sdslen(request->data), "{params: {offset: %u}}", &uint_buf1);
            if (je == 1) {
                data = mympd_api_bookmark_list(config, data, request->method, request->id, uint_buf1);
            }
            break;
        case MYMPD_API_COVERCACHE_CROP:
            clear_covercache(config, -1);
            data = jsonrpc_respond_message(data, request->method, request->id, "Successfully croped covercache", false);
            break;
        case MYMPD_API_COVERCACHE_CLEAR:
            clear_covercache(config, 0);
            data = jsonrpc_respond_message(data, request->method, request->id, "Successfully cleared covercache", false);
            break;
        default:
            data = jsonrpc_respond_message(data, request->method, request->id, "Unknown request", true);
            LOG_ERROR("Unknown API request: %.*s", sdslen(request->data), request->data);
    }

    FREE_PTR(p_charbuf1);
    FREE_PTR(p_charbuf2);
    FREE_PTR(p_charbuf3);

    if (sdslen(data) == 0) {
        data = jsonrpc_start_phrase(data, request->method, request->id, "No response for method %{method}", true);
        data = tojson_char(data, "method", request->method, false);
        data = jsonrpc_end_phrase(data);
        LOG_ERROR("No response for cmd_id %u", request->cmd_id);
    }
    response->data = data;
    LOG_DEBUG("Push response to queue for connection %lu: %s", request->conn_id, response->data);
    tiny_queue_push(web_server_queue, response);
    free_request(request);
}
