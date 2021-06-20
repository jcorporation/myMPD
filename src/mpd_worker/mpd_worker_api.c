/*
 SPDX-License-Identifier: GPL-2.0-or-later
 myMPD (c) 2018-2021 Juergen Mang <mail@jcgames.de>
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
#include "../dist/src/rax/rax.h"
#include "../sds_extras.h"
#include "../../dist/src/frozen/frozen.h"
#include "../list.h"
#include "mympd_config_defs.h"
#include "../utility.h"
#include "../api.h"
#include "../log.h"
#include "../tiny_queue.h"
#include "../global.h"
#include "../mympd_state.h"
#include "../mpd_shared.h"
#include "mpd_worker_utility.h"
#include "mpd_worker_smartpls.h"
#include "mpd_worker_cache.h"
#include "mpd_worker_api.h"

//public functions
void mpd_worker_api(struct t_mpd_worker_state *mpd_worker_state) {
    t_work_request *request = mpd_worker_state->request;
    bool rc;
    bool bool_buf1;
    bool async = false;
    int je;
    char *p_charbuf1 = NULL;
    char *p_charbuf2 = NULL;
    char *p_charbuf3 = NULL;
    char *p_charbuf4 = NULL;
    char *p_charbuf5 = NULL;

    MYMPD_LOG_INFO("MPD WORKER API request (%lld)(%ld) %s: %s", request->conn_id, request->id, request->method, request->data);
    //create response struct
    t_work_result *response = create_result(request);
    
    switch(request->cmd_id) {
        case MYMPD_API_SMARTPLS_UPDATE_ALL:
            if (mpd_worker_state->smartpls == false) {
                send_jsonrpc_notify("playlist", "error", "Smart playlists are disabled");
                async = true;
                free_result(response);
                free_request(request);
                break;
            }
            je = json_scanf(request->data, sdslen(request->data), "{params: {force: %B}}", &bool_buf1);
            if (je == 1) {
                response->data = jsonrpc_respond_message(response->data, request->method, request->id, false, 
                    "playlist", "info", "Smart playlists update started");
                if (request->conn_id > -1) {
                    MYMPD_LOG_DEBUG("Push response to queue for connection %lld: %s", request->conn_id, response->data);
                    tiny_queue_push(web_server_queue, response, 0);
                }
                else {
                    free_result(response);
                }
                free_request(request);
                rc = mpd_worker_smartpls_update_all(mpd_worker_state, bool_buf1);
                if (rc == true) {
                    send_jsonrpc_notify("playlist", "info", "Smart playlists updated");
                }
                else {
                    send_jsonrpc_notify("playlist", "error", "Smart playlists update failed");
                }
                async = true;
            }
            break;
        case MYMPD_API_SMARTPLS_UPDATE:
            if (mpd_worker_state->smartpls == false) {
                send_jsonrpc_notify("playlist", "error", "Smart playlists are disabled");
                async = true;
                break;
            }
            je = json_scanf(request->data, sdslen(request->data), "{params: {plist: %Q}}", &p_charbuf1);
            if (je == 1) {
                rc = mpd_worker_smartpls_update(mpd_worker_state, p_charbuf1);
                if (rc == true) {
                    response->data = jsonrpc_respond_message_phrase(response->data, request->method, request->id, false,
                        "playlist", "info", "Smart playlist %{playlist} updated", 2, "playlist", p_charbuf1);
                }
                else {
                    response->data = jsonrpc_respond_message_phrase(response->data, request->method, request->id, true,
                        "playlist", "error", "Updating smart playlist %{playlist} failed", 2, "playlist", p_charbuf1);
                }
            }
            break;
        case MYMPD_API_CACHES_CREATE:
            mpd_worker_cache_init(mpd_worker_state);
            async = true;
            free_request(request);
            free_result(response);
            break;
        default:
            response->data = jsonrpc_respond_message(response->data, request->method, request->id, true, "general", "error", "Unknown request");
            MYMPD_LOG_ERROR("Unknown API request: %.*s", sdslen(request->data), request->data);
    }
    FREE_PTR(p_charbuf1);
    FREE_PTR(p_charbuf2);
    FREE_PTR(p_charbuf3);                    
    FREE_PTR(p_charbuf4);
    FREE_PTR(p_charbuf5);

    if (async == true) {
        return;
    }
    
    if (sdslen(response->data) == 0) {
        response->data = jsonrpc_respond_message_phrase(response->data, request->method, request->id, true, 
            "general", "error", "No response for method %{method}", 2, "method", request->method);
        MYMPD_LOG_ERROR("No response for method \"%s\"", request->method);
    }
    if (request->conn_id == -2) {
        MYMPD_LOG_DEBUG("Push response to mympd_script_queue for thread %ld: %s", request->id, response->data);
        tiny_queue_push(mympd_script_queue, response, request->id);
    }
    else if (request->conn_id > -1) {
        MYMPD_LOG_DEBUG("Push response to queue for connection %lld: %s", request->conn_id, response->data);
        tiny_queue_push(web_server_queue, response, 0);
    }
    else {
        free_result(response);
    }
    free_request(request);
}
