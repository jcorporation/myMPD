/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2024 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include "compile_time.h"
#include "src/lib/api.h"

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
        case MYMPD_API_CACHE_DISK_CLEAR:
        case MYMPD_API_CACHE_DISK_CROP:
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
        case MYMPD_API_SCRIPT_VAR_DELETE:
        case MYMPD_API_SCRIPT_VAR_LIST:
        case MYMPD_API_SCRIPT_VAR_SET:
            return true;
        default:
            return false;
    }
}

/**
 * Defines methods that are public
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
 * Defines methods that are accessible by scripts
 * @param cmd_id myMPD API method
 * @return true if public else false
 */
bool is_script_api_method(enum mympd_cmd_ids cmd_id) {
    switch(cmd_id) {
        case INTERNAL_API_SCRIPT_INIT:
        case INTERNAL_API_JUKEBOX_CREATED:
        case INTERNAL_API_JUKEBOX_ERROR:
            return true;
        default:
        if (cmd_id <= INTERNAL_API_COUNT ||
            cmd_id >= TOTAL_API_COUNT)
        {
            return false;
        }
    }
    return true;
}

/**
 * Defines methods that should work with no mpd connection,
 * this is necessary for correct startup and changing mpd connection settings.
 * The list is not complete.
 * @param cmd_id myMPD API method
 * @return true if method works with no mpd connection else false
 */
bool is_mympd_only_api_method(enum mympd_cmd_ids cmd_id) {
    switch(cmd_id) {
        case MYMPD_API_CONNECTION_SAVE:
        case MYMPD_API_HOME_ICON_LIST:
        case MYMPD_API_SCRIPT_LIST:
        case MYMPD_API_SETTINGS_GET:
        case MYMPD_API_CACHE_DISK_CLEAR:
        case MYMPD_API_CACHE_DISK_CROP:
        case MYMPD_API_WEBRADIODB_UPDATE:
            return true;
        default:
            return false;
    }
}

/**
 * Sends a websocket message to all clients in a partition
 * @param message the message to send
 * @param partition mpd partition
 */
void ws_notify(sds message, const char *partition) {
    MYMPD_LOG_DEBUG(partition, "Push websocket notify to queue: \"%s\"", message);
    struct t_work_response *response = create_response_new(RESPONSE_TYPE_NOTIFY_PARTITION, 0, 0, INTERNAL_API_WEBSERVER_NOTIFY, partition);
    response->data = sds_replace(response->data, message);
    mympd_queue_push(web_server_queue, response, 0);
}

/**
 * Sends a websocket message to a client
 * @param message the message to send
 * @param request_id the jsonrpc id of the client
 */
void ws_notify_client(sds message, unsigned request_id) {
    MYMPD_LOG_DEBUG(NULL, "Push websocket notify to queue: \"%s\"", message);
    struct t_work_response *response = create_response_new(RESPONSE_TYPE_NOTIFY_CLIENT, 0, request_id, INTERNAL_API_WEBSERVER_NOTIFY, MPD_PARTITION_ALL);
    response->data = sds_replace(response->data, message);
    mympd_queue_push(web_server_queue, response, 0);
}

void ws_script_dialog(sds message, unsigned request_id) {
    MYMPD_LOG_DEBUG(NULL, "Push websocket notify to queue: \"%s\"", message);
    struct t_work_response *response = create_response_new(RESPONSE_TYPE_SCRIPT_DIALOG, 0, request_id, INTERNAL_API_WEBSERVER_NOTIFY, MPD_PARTITION_ALL);
    response->data = sds_replace(response->data, message);
    mympd_queue_push(web_server_queue, response, 0);
}

/**
 * Mallocs and initializes a t_work_response struct, as reply of the provided request
 * @param request the request the ids are copied
 * @return the initialized t_work_response struct
 */
struct t_work_response *create_response(struct t_work_request *request) {
    enum work_response_types type = RESPONSE_TYPE_DEFAULT;
    switch(request->type) {
        case REQUEST_TYPE_DEFAULT: type = RESPONSE_TYPE_DEFAULT; break;
        case REQUEST_TYPE_SCRIPT:  type = RESPONSE_TYPE_SCRIPT; break;
        case REQUEST_TYPE_NOTIFY_PARTITION: type = RESPONSE_TYPE_NOTIFY_PARTITION; break;
        case REQUEST_TYPE_DISCARD: type = RESPONSE_TYPE_DISCARD; break;
    }
    struct t_work_response *response = create_response_new(type, request->conn_id, request->id, request->cmd_id, request->partition);
    return response;
}

/**
 * Mallocs and initializes a t_work_response struct
 * @param type work response type
 * @param conn_id connection id (from webserver)
 * @param request_id id for the request
 * @param cmd_id myMPD API method
 * @param partition mpd partition
 * @return the initialized t_work_response struct
 */
struct t_work_response *create_response_new(enum work_response_types type, unsigned long conn_id, unsigned request_id, enum mympd_cmd_ids cmd_id, const char *partition) {
    struct t_work_response *response = malloc_assert(sizeof(struct t_work_response));
    response->type = type;
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
 * @param type work request type
 * @param conn_id connection id (from webserver)
 * @param request_id id for the request
 * @param cmd_id myMPD API method
 * @param data jsonrpc request, if NULL the start of the request is added
 * @param partition mpd partition
 * @return the initialized t_work_request struct
 */
struct t_work_request *create_request(enum work_request_types type, unsigned long conn_id, unsigned request_id, enum mympd_cmd_ids cmd_id, const char *data, const char *partition) {
    struct t_work_request *request = malloc_assert(sizeof(struct t_work_request));
    request->type = type;
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
 * @return true on success, else false
 */
bool push_response(struct t_work_response *response) {
    switch(response->type) {
        case RESPONSE_TYPE_DEFAULT:
        case RESPONSE_TYPE_NOTIFY_CLIENT:
        case RESPONSE_TYPE_NOTIFY_PARTITION:
        case RESPONSE_TYPE_PUSH_CONFIG:
        case RESPONSE_TYPE_SCRIPT_DIALOG:
            MYMPD_LOG_DEBUG(NULL, "Push response to webserver queue for connection %lu: %s", response->conn_id, response->data);
            return mympd_queue_push(web_server_queue, response, 0);
        case RESPONSE_TYPE_RAW:
            MYMPD_LOG_DEBUG(NULL, "Push raw response to webserver queue for connection %lu with %lu bytes", response->conn_id, (unsigned long)sdslen(response->data));
            return mympd_queue_push(web_server_queue, response, 0);
        case RESPONSE_TYPE_SCRIPT:
            #ifdef MYMPD_ENABLE_LUA
                MYMPD_LOG_DEBUG(NULL, "Push response to script_worker_queue for thread %u: %s", response->id, response->data);
                return mympd_queue_push(script_worker_queue, response, response->id);
            #endif
        case RESPONSE_TYPE_DISCARD:
            // discard response
            free_response(response);
            return true;
    }
    // this should not appear
    MYMPD_LOG_ERROR(NULL, "Invalid response type for connection %lu: %s", response->conn_id, response->data);
    free_response(response);
    return false;
}

/**
 * Pushes the request to a queue
 * @param request pointer to request struct to push
 * @param id request id
 * @return true on success, else false
 */
bool push_request(struct t_work_request *request, unsigned id) {
    switch(request->cmd_id) {
        case INTERNAL_API_SCRIPT_EXECUTE:
        case INTERNAL_API_SCRIPT_POST_EXECUTE:
        case MYMPD_API_SCRIPT_EXECUTE:
        case MYMPD_API_SCRIPT_GET:
        case MYMPD_API_SCRIPT_LIST:
        case MYMPD_API_SCRIPT_RM:
        case MYMPD_API_SCRIPT_SAVE:
        case MYMPD_API_SCRIPT_VALIDATE:
        case MYMPD_API_SCRIPT_VAR_DELETE:
        case MYMPD_API_SCRIPT_VAR_LIST:
        case MYMPD_API_SCRIPT_VAR_SET:
            #ifdef MYMPD_ENABLE_LUA
                //forward API request to script thread
                return mympd_queue_push(script_queue, request, id);
            #endif
        default:
            //forward API request to mympd_api thread
            return mympd_queue_push(mympd_api_queue, request, id);
    }
}
