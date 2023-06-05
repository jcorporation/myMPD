/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2023 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include "compile_time.h"
#include "src/lib/api.h"

#include "dist/libmympdclient/include/mpd/client.h"
#include "src/lib/log.h"
#include "src/lib/mem.h"
#include "src/lib/msg_queue.h"
#include "src/lib/sds_extras.h"

#include <string.h>

static const char *mympd_cmd_strs[] = { MYMPD_CMDS(GEN_STR) };

/**
 * Converts a string to the mympd_cmd_ids enum
 * @param cmd string to convert
 * @return enum mympd_cmd_ids
 */
enum mympd_cmd_ids get_cmd_id(const char *cmd) {
    for (unsigned i = 0; i < TOTAL_API_COUNT; i++) {
        if (strcmp(cmd, mympd_cmd_strs[i]) == 0) {
            return i;
        }
    }
    return GENERAL_API_UNKNOWN;
}

/**
 * Converts the mympd_cmd_ids enum to the string
 * @param cmd_id myMPD API method
 * @return the API method as string
 */
const char *get_cmd_id_method_name(enum mympd_cmd_ids cmd_id) {
    if (cmd_id >= TOTAL_API_COUNT) {
        return NULL;
    }
    return mympd_cmd_strs[cmd_id];
}

/**
 * Defines methods that need authentication if a pin is set.
 * @param cmd_id myMPD API method
 * @return true if protected else false
 */
bool is_protected_api_method(enum mympd_cmd_ids cmd_id) {
    switch(cmd_id) {
        case MYMPD_API_CONNECTION_SAVE:
        case MYMPD_API_COVERCACHE_CLEAR:
        case MYMPD_API_COVERCACHE_CROP:
        case MYMPD_API_MOUNT_MOUNT:
        case MYMPD_API_MOUNT_UNMOUNT:
        case MYMPD_API_PARTITION_NEW:
        case MYMPD_API_PARTITION_RM:
        case MYMPD_API_PARTITION_SAVE:
        case MYMPD_API_PARTITION_OUTPUT_MOVE:
        case MYMPD_API_PLAYER_OUTPUT_ATTRIBUTES_SET:
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

/**
 * Defines methods that are internal
 * @param cmd_id myMPD API method
 * @return true if public else false
 */
bool is_public_api_method(enum mympd_cmd_ids cmd_id) {
    if (cmd_id <= INTERNAL_API_COUNT ||
        cmd_id >= TOTAL_API_COUNT)
    {
        return false;
    }
    return true;
}

/**
 * Defines methods that should work with no mpd connection,
 * this is necessary for correct startup and changing mpd connection settings.
 * @param cmd_id myMPD API method
 * @return true if method works with no mpd connection else false
 */
bool is_mympd_only_api_method(enum mympd_cmd_ids cmd_id) {
    switch(cmd_id) {
        case MYMPD_API_CONNECTION_SAVE:
        case MYMPD_API_HOME_ICON_LIST:
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
 * @param partition mpd partition
 */
void ws_notify(sds message, const char *partition) {
    MYMPD_LOG_DEBUG(partition, "Push websocket notify to queue: \"%s\"", message);
    struct t_work_response *response = create_response_new(0, 0, INTERNAL_API_WEBSERVER_NOTIFY, partition);
    response->data = sds_replace(response->data, message);
    mympd_queue_push(web_server_queue, response, 0);
}

/**
 * Mallocs and initializes a t_work_response struct, copies the ids from the request struct
 * @param request the request the ids are copied
 * @return the initialized t_work_response struct
 */
struct t_work_response *create_response(struct t_work_request *request) {
    struct t_work_response *response = create_response_new(request->conn_id, request->id, request->cmd_id, request->partition);
    return response;
}

/**
 * Mallocs and initializes a t_work_response struct
 * @param conn_id connection id (from webserver)
 * @param request_id id for the request
 * @param cmd_id myMPD API method
 * @param partition mpd partition
 * @return the initialized t_work_response struct
 */
struct t_work_response *create_response_new(long long conn_id, long request_id, enum mympd_cmd_ids cmd_id, const char *partition) {
    struct t_work_response *response = malloc_assert(sizeof(struct t_work_response));
    response->conn_id = conn_id;
    response->id = request_id;
    response->cmd_id = cmd_id;
    response->data = sdsempty();
    response->binary = sdsempty();
    response->extra = NULL;
    response->partition = sdsnew(partition);
    return response;
}

/**
 * Mallocs and initializes a t_work_request struct
 * @param conn_id connection id (from webserver)
 * @param request_id id for the request
 * @param cmd_id myMPD API method
 * @param data jsonrpc request, if NULL the start of the request is added
 * @param partition mpd partition
 * @return the initialized t_work_request struct
 */
struct t_work_request *create_request(long long conn_id, long request_id, enum mympd_cmd_ids cmd_id, const char *data, const char *partition) {
    struct t_work_request *request = malloc_assert(sizeof(struct t_work_request));
    request->conn_id = conn_id;
    request->cmd_id = cmd_id;
    request->id = request_id;
    if (data == NULL) {
        request->data = sdscatfmt(sdsempty(), "{\"jsonrpc\":\"2.0\",\"id\":0,\"method\":\"%s\",\"params\":{", get_cmd_id_method_name(cmd_id));
    }
    else {
        request->data = sdsnew(data);
    }
    request->extra = NULL;
    request->partition = sdsnew(partition);
    return request;
}

/**
 * Frees the request struct
 * @param request request struct to free
 */
void free_request(struct t_work_request *request) {
    if (request != NULL) {
        FREE_SDS(request->data);
        FREE_SDS(request->partition);
        FREE_PTR(request);
    }
}

/**
 * Frees the response struct
 * @param response response struct to free
 */
void free_response(struct t_work_response *response) {
    if (response != NULL) {
        FREE_SDS(response->data);
        FREE_SDS(response->binary);
        FREE_SDS(response->partition);
        FREE_PTR(response);
    }
}

/**
 * Pushes the response to a queue or frees it
 * @param response pointer to response struct to push
 * @param request_id request id
 * @param conn_id connection id
 * @return true on success, else false
 */
bool push_response(struct t_work_response *response, long request_id, long long conn_id) {
    if (conn_id == -2) {
        MYMPD_LOG_DEBUG(NULL, "Push response to mympd_script_queue for thread %ld: %s", request_id, response->data);
        return mympd_queue_push(mympd_script_queue, response, request_id);
    }
    if (conn_id > -1) {
        MYMPD_LOG_DEBUG(NULL, "Push response to queue for connection %lld: %s", conn_id, response->data);
        return mympd_queue_push(web_server_queue, response, 0);
    }
    free_response(response);
    return true;
}
