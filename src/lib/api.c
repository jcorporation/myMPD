/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2022 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include "mympd_config_defs.h"
#include "api.h"

#include "../../dist/mongoose/mongoose.h"
#include "log.h"
#include "lua_mympd_state.h"
#include "mem.h"
#include "sds_extras.h"

#include <mpd/client.h>
#include <string.h>

//global variables
_Atomic int worker_threads;
sig_atomic_t s_signal_received;
struct t_mympd_queue *web_server_queue;
struct t_mympd_queue *mympd_api_queue;
struct t_mympd_queue *mympd_script_queue;

//method to id and reverse
static const char *mympd_cmd_strs[] = { MYMPD_CMDS(GEN_STR) };

enum mympd_cmd_ids get_cmd_id(const char *cmd) {
    for (unsigned i = 0; i < TOTAL_API_COUNT; i++) {
        if (strcmp(cmd, mympd_cmd_strs[i]) == 0) {
            return i;
        }
    }
    return GENERAL_API_UNKNOWN;
}

const char *get_cmd_id_method_name(enum mympd_cmd_ids cmd_id) {
    if (cmd_id >= TOTAL_API_COUNT) {
        return NULL;
    }
    return mympd_cmd_strs[cmd_id];
}

//defines methods that need authentication if a pin is set
bool is_protected_api_method(enum mympd_cmd_ids cmd_id) {
    switch(cmd_id) {
        case MYMPD_API_CONNECTION_SAVE:
        case MYMPD_API_COVERCACHE_CLEAR:
        case MYMPD_API_COVERCACHE_CROP:
        case MYMPD_API_MOUNT_MOUNT:
        case MYMPD_API_MOUNT_UNMOUNT:
        case MYMPD_API_PARTITION_NEW:
        case MYMPD_API_PARTITION_RM:
        case MYMPD_API_PARTITION_OUTPUT_MOVE:
        case MYMPD_API_PLAYER_OUTPUT_ATTRIBUTS_SET:
        case MYMPD_API_PLAYLIST_RM_ALL:
        case MYMPD_API_SESSION_LOGOUT:
        case MYMPD_API_SESSION_VALIDATE:
        case MYMPD_API_SETTINGS_SET:
        case MYMPD_API_SCRIPT_RM:
        case MYMPD_API_SCRIPT_SAVE:
        case MYMPD_API_TIMER_RM:
        case MYMPD_API_TIMER_SAVE:
        case MYMPD_API_TIMER_TOGGLE:
        case MYMPD_API_TRIGGER_RM:
        case MYMPD_API_TRIGGER_SAVE:
        case MYMPD_API_LOGLEVEL:
            return true;
        default:
            return false;
    }
}

//defines methods that are internal
bool is_public_api_method(enum mympd_cmd_ids cmd_id) {
    if (cmd_id <= INTERNAL_API_COUNT ||
        cmd_id >= TOTAL_API_COUNT)
    {
        return false;
    }
    return true;
}

//defines methods that should work with no mpd connection
//this is necessary for correct startup and changing mpd connection settings
bool is_mympd_only_api_method(enum mympd_cmd_ids cmd_id) {
    switch(cmd_id) {
        case MYMPD_API_CONNECTION_SAVE:
        case MYMPD_API_HOME_LIST:
        case MYMPD_API_SCRIPT_LIST:
        case MYMPD_API_SETTINGS_GET:
            return true;
        default:
            return false;
    }
}

/**
 * Sends a websocket notification to the browser
 * @param message the message to send
 */
void ws_notify(sds message) {
    MYMPD_LOG_DEBUG("Push websocket notify to queue: \"%s\"", message);
    struct t_work_result *response = create_result_new(0, 0, INTERNAL_API_WEBSERVER_NOTIFY);
    response->data = sds_replace(response->data, message);
    mympd_queue_push(web_server_queue, response, 0);
}

struct t_work_result *create_result(struct t_work_request *request) {
    struct t_work_result *response = create_result_new(request->conn_id, request->id, request->cmd_id);
    return response;
}

struct t_work_result *create_result_new(long long conn_id, long request_id, enum mympd_cmd_ids cmd_id) {
    struct t_work_result *response = malloc_assert(sizeof(struct t_work_result));
    response->conn_id = conn_id;
    response->id = request_id;
    response->cmd_id = cmd_id;
    const char *method = get_cmd_id_method_name(cmd_id);
    response->method = sdsnew(method);
    response->data = sdsempty();
    response->binary = sdsempty();
    response->extra = NULL;
    return response;
}

struct t_work_request *create_request(long long conn_id, long request_id, enum mympd_cmd_ids cmd_id, const char *data) {
    struct t_work_request *request = malloc_assert(sizeof(struct t_work_request));
    request->conn_id = conn_id;
    request->cmd_id = cmd_id;
    request->id = request_id;
    const char *method = get_cmd_id_method_name(cmd_id);
    request->method = sdsnew(method);
    if (data == NULL) {
        request->data = sdscatfmt(sdsempty(), "{\"jsonrpc\":\"2.0\",\"id\":0,\"method\":\"%s\",\"params\":{", method);
    }
    else {
        request->data = sdsnew(data);
    }
    request->extra = NULL;
    return request;
}

void free_request(struct t_work_request *request) {
    if (request != NULL) {
        FREE_SDS(request->data);
        FREE_SDS(request->method);
        FREE_PTR(request);
    }
}

void free_result(struct t_work_result *result) {
    if (result != NULL) {
        FREE_SDS(result->data);
        FREE_SDS(result->method);
        FREE_SDS(result->binary);
        FREE_PTR(result);
    }
}

int expire_result_queue(struct t_mympd_queue *queue, time_t age) {
    struct t_work_result *response = NULL;
    int i = 0;
    while ((response = mympd_queue_expire(queue, age)) != NULL) {
        if (response->extra != NULL) {
            if (response->cmd_id == INTERNAL_API_SCRIPT_INIT) {
                lua_mympd_state_free(response->extra);
            }
            else {
               FREE_PTR(response->extra);
            }
        }
        free_result(response);
        response = NULL;
        i++;
    }
    return i;
}

int expire_request_queue(struct t_mympd_queue *queue, time_t age) {
    struct t_work_request *request = NULL;
    int i = 0;
    while ((request = mympd_queue_expire(queue, age)) != NULL) {
        if (request->extra != NULL) {
            if (request->cmd_id == INTERNAL_API_SCRIPT_INIT) {
                lua_mympd_state_free(request->extra);
            }
            else {
                FREE_PTR(request->extra);
            }
        }
        free_request(request);
        request = NULL;
        i++;
    }
    return i;
}
