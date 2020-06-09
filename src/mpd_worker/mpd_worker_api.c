/*
 SPDX-License-Identifier: GPL-2.0-or-later
 myMPD (c) 2018-2020 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <assert.h>
#include <inttypes.h>
#include <mpd/client.h>

#include "../../dist/src/sds/sds.h"
#include "../sds_extras.h"
#include "../../dist/src/frozen/frozen.h"
#include "../list.h"
#include "config_defs.h"
#include "../utility.h"
#include "../api.h"
#include "../log.h"
#include "../tiny_queue.h"
#include "../global.h"
#include "../mpd_shared/mpd_shared_typedefs.h"
#include "../mpd_shared.h"
#include "mpd_worker_utility.h"
#include "mpd_worker_api.h"

//private definitions
static bool mpd_worker_api_settings_set(t_mpd_worker_state *mpd_worker_state, struct json_token *key, 
                          struct json_token *val, bool *mpd_host_changed, bool *check_mpd_error);

//public functions
void mpd_worker_api(t_config *config, t_mpd_worker_state *mpd_worker_state, void *arg_request) {
    t_work_request *request = (t_work_request*) arg_request;
    bool rc;
    char *p_charbuf1 = NULL;
    char *p_charbuf2 = NULL;
    char *p_charbuf3 = NULL;
    char *p_charbuf4 = NULL;
    char *p_charbuf5 = NULL;

    LOG_VERBOSE("MPD WORKER API request (%d)(%d) %s: %s", request->conn_id, request->id, request->method, request->data);
    //create response struct
    t_work_result *response = create_result(request);
    
    switch(request->cmd_id) {
        case MYMPD_API_SETTINGS_SET: {
            void *h = NULL;
            struct json_token key;
            struct json_token val;
            rc = true;
            bool mpd_host_changed = false;
            bool check_mpd_error = false;
            sds notify_buffer = sdsempty();
            while ((h = json_next_key(request->data, sdslen(request->data), h, ".params", &key, &val)) != NULL) {
                rc = mpd_worker_api_settings_set(mpd_worker_state, &key, &val, &mpd_host_changed, &check_mpd_error);
                if ((check_mpd_error == true && check_error_and_recover2(mpd_worker_state->mpd_state, &notify_buffer, request->method, request->id, true) == false)
                    || rc == false)
                {
                    if (sdslen(notify_buffer) > 0) {
                        ws_notify(notify_buffer);
                    }
                    break;
                }
            }
            sdsfree(notify_buffer);
            if (rc == true) {
                if (mpd_host_changed == true) {
                    //reconnect with new settings
                    mpd_worker_state->mpd_state->conn_state = MPD_DISCONNECT;
                }
                response->data = jsonrpc_respond_ok(response->data, request->method, request->id);
            }
            else {
                response->data = jsonrpc_start_phrase(response->data, request->method, request->id, "Can't save setting %{setting}", true);
                response->data = tojson_char_len(response->data, "setting", val.ptr, val.len, false);
                response->data = jsonrpc_end_phrase(response->data);
            }
            break;
        }
        default:
            response->data = jsonrpc_respond_message(response->data, request->method, request->id, "Unknown request", true);
            LOG_ERROR("Unknown API request: %.*s", sdslen(request->data), request->data);
    }
    FREE_PTR(p_charbuf1);
    FREE_PTR(p_charbuf2);
    FREE_PTR(p_charbuf3);                    
    FREE_PTR(p_charbuf4);
    FREE_PTR(p_charbuf5);

    if (sdslen(response->data) == 0) {
        response->data = jsonrpc_start_phrase(response->data, request->method, request->id, "No response for method %{method}", true);
        response->data = tojson_char(response->data, "method", request->method, false);
        response->data = jsonrpc_end_phrase(response->data);
        LOG_ERROR("No response for cmd_id %u", request->cmd_id);
    }
    if (request->conn_id > -1) {
        LOG_DEBUG("Push response to queue for connection %lu: %s", request->conn_id, response->data);
        tiny_queue_push(web_server_queue, response);
    }
    else {
        free_result(response);
    }
    free_request(request);
    //prevent unused paramter warning
    (void) config;
}

//private functions
static bool mpd_worker_api_settings_set(t_mpd_worker_state *mpd_worker_state, struct json_token *key, 
                          struct json_token *val, bool *mpd_host_changed, bool *check_mpd_error)
{
    bool rc = true;
    char *crap;
    sds settingvalue = sdscatlen(sdsempty(), val->ptr, val->len);

    *check_mpd_error = false;
    LOG_DEBUG("Parse setting \"%.*s\" with value \"%.*s\"", key->len, key->ptr, val->len, val->ptr);
    if (strncmp(key->ptr, "mpdPass", key->len) == 0) {
        if (strncmp(val->ptr, "dontsetpassword", val->len) != 0) {
            *mpd_host_changed = true;
            mpd_worker_state->mpd_state->mpd_pass = sdsreplacelen(mpd_worker_state->mpd_state->mpd_pass, settingvalue, sdslen(settingvalue));
        }
        else {
            sdsfree(settingvalue);
            return true;
        }
    }
    else if (strncmp(key->ptr, "mpdHost", key->len) == 0) {
        if (strncmp(val->ptr, mpd_worker_state->mpd_state->mpd_host, val->len) != 0) {
            *mpd_host_changed = true;
            mpd_worker_state->mpd_state->mpd_host = sdsreplacelen(mpd_worker_state->mpd_state->mpd_host, settingvalue, sdslen(settingvalue));
        }
    }
    else if (strncmp(key->ptr, "mpdPort", key->len) == 0) {
        int mpd_port = strtoimax(settingvalue, &crap, 10);
        if (mpd_worker_state->mpd_state->mpd_port != mpd_port) {
            *mpd_host_changed = true;
            mpd_worker_state->mpd_state->mpd_port = mpd_port;
        }
    }
    sdsfree(settingvalue);
    return rc;
}
