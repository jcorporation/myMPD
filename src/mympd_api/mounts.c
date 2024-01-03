/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2024 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include "compile_time.h"
#include "src/mympd_api/mounts.h"

#include "src/lib/jsonrpc.h"
#include "src/lib/sds_extras.h"
#include "src/mpd_client/errorhandler.h"

#include <string.h>

/**
 * Prints the list of mounted mpd uris as jsonrpc response
 * @param partition_state pointer to partition state
 * @param buffer already allocated sds string to append the response
 * @param request_id jsonrpc request id
 * @return pointer to buffer
 */
sds mympd_api_mounts_list(struct t_partition_state *partition_state, sds buffer, unsigned request_id) {
    enum mympd_cmd_ids cmd_id = MYMPD_API_MOUNT_LIST;
    if (mpd_send_list_mounts(partition_state->conn)) {
        buffer = jsonrpc_respond_start(buffer, cmd_id, request_id);
        buffer = sdscat(buffer, "\"data\":[");
        unsigned entity_count = 0;
        struct mpd_mount *mount;
        while ((mount = mpd_recv_mount(partition_state->conn)) != NULL) {
            const char *uri = mpd_mount_get_uri(mount);
            const char *storage = mpd_mount_get_storage(mount);
            if (uri != NULL && storage != NULL) {
                if (entity_count++) {
                    buffer = sdscatlen(buffer, ",", 1);
                }
                buffer = sdscatlen(buffer, "{", 1);
                buffer = tojson_char(buffer, "mountPoint", uri, true);
                buffer = tojson_char(buffer, "mountUrl", storage, false);
                buffer = sdscatlen(buffer, "}", 1);
            }
            mpd_mount_free(mount);
        }
        buffer = sdscatlen(buffer, "],", 2);
        buffer = tojson_uint(buffer, "totalEntities", entity_count, true);
        buffer = tojson_uint(buffer, "returnedEntities", entity_count, false);
        buffer = jsonrpc_end(buffer);
    }
    mpd_response_finish(partition_state->conn);
    mympd_check_error_and_recover_respond(partition_state, &buffer, cmd_id, request_id, "mpd_send_list_mounts");

    return buffer;
}

/**
 * Prints the list of url handlers as jsonrpc response
 * @param partition_state pointer to partition state
 * @param buffer already allocated sds string to append the response
 * @param request_id jsonrpc request id
 * @return pointer to buffer
 */
sds mympd_api_mounts_urlhandler_list(struct t_partition_state *partition_state, sds buffer, unsigned request_id) {
    enum mympd_cmd_ids cmd_id = MYMPD_API_MOUNT_URLHANDLER_LIST;
    if (mpd_send_command(partition_state->conn, "urlhandlers", NULL)) {
        buffer = jsonrpc_respond_start(buffer, cmd_id, request_id);
        buffer = sdscat(buffer, "\"data\":[");
        unsigned entity_count = 0;
        struct mpd_pair *pair;
        while ((pair = mpd_recv_pair(partition_state->conn)) != NULL) {
            if (entity_count++) {
                buffer = sdscatlen(buffer, ",", 1);
            }
            buffer = sds_catjson(buffer, pair->value, strlen(pair->value));
            mpd_return_pair(partition_state->conn, pair);
        }
        buffer = sdscatlen(buffer, "],", 2);
        buffer = tojson_uint(buffer, "totalEntities", entity_count, true);
        buffer = tojson_uint(buffer, "returnedEntities", entity_count, false);
        buffer = jsonrpc_end(buffer);
    }
    mpd_response_finish(partition_state->conn);
    mympd_check_error_and_recover_respond(partition_state, &buffer, cmd_id, request_id, "mpd_send_command");
    return buffer;
}

/**
 * Prints the list of neighbors as jsonrpc response
 * @param partition_state pointer to partition state
 * @param buffer already allocated sds string to append the response
 * @param request_id jsonrpc request id
 * @return pointer to buffer
 */
sds mympd_api_mounts_neighbor_list(struct t_partition_state *partition_state, sds buffer, unsigned request_id) {
    enum mympd_cmd_ids cmd_id = MYMPD_API_MOUNT_NEIGHBOR_LIST;
    if (mpd_send_list_neighbors(partition_state->conn)) {
        buffer = jsonrpc_respond_start(buffer, cmd_id, request_id);
        buffer = sdscat(buffer, "\"data\":[");
        unsigned entity_count = 0;
        struct mpd_neighbor *neighbor;
        while ((neighbor = mpd_recv_neighbor(partition_state->conn)) != NULL) {
            const char *uri = mpd_neighbor_get_uri(neighbor);
            //upnp uris can not be mounted
            if (strncmp(uri, "upnp://", 7) != 0) {
                if (entity_count++) {
                    buffer = sdscatlen(buffer, ",", 1);
                }
                buffer = sdscatlen(buffer, "{", 1);
                buffer = tojson_char(buffer, "uri", uri, true);
                buffer = tojson_char(buffer, "displayName", mpd_neighbor_get_display_name(neighbor), false);
                buffer = sdscatlen(buffer, "}", 1);
            }
            mpd_neighbor_free(neighbor);
        }
        buffer = sdscatlen(buffer, "],", 2);
        buffer = tojson_uint(buffer, "totalEntities", entity_count, true);
        buffer = tojson_uint(buffer, "returnedEntities", entity_count, false);
        buffer = jsonrpc_end(buffer);
    }
    mpd_response_finish(partition_state->conn);
    mympd_check_error_and_recover_respond(partition_state, &buffer, cmd_id, request_id, "mpd_send_list_neighbors");
    return buffer;
}
