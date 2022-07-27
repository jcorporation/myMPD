/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2022 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include "compile_time.h"
#include "mounts.h"

#include "../lib/jsonrpc.h"
#include "../lib/sds_extras.h"
#include "../mpd_client/errorhandler.h"

#include <string.h>

//public functions
sds mympd_api_mounts_list(struct t_partition_state *partition_state, sds buffer, long request_id) {
    enum mympd_cmd_ids cmd_id = MYMPD_API_MOUNT_LIST;
    bool rc = mpd_send_list_mounts(partition_state->conn);
    if (mympd_check_rc_error_and_recover_respond(partition_state, &buffer, cmd_id, request_id, rc, "mpd_send_list_mounts") == false) {
        return buffer;
    }

    buffer = jsonrpc_respond_start(buffer, cmd_id, request_id);
    buffer = sdscat(buffer, "\"data\":[");
    long entity_count = 0;
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
    buffer = tojson_long(buffer, "totalEntities", entity_count, true);
    buffer = tojson_long(buffer, "returnedEntities", entity_count, false);
    buffer = jsonrpc_respond_end(buffer);

    mpd_response_finish(partition_state->conn);
    if (mympd_check_error_and_recover_respond(partition_state, &buffer, cmd_id, request_id) == false) {
        return buffer;
    }

    return buffer;
}

sds mympd_api_mounts_urlhandler_list(struct t_partition_state *partition_state, sds buffer, long request_id) {
    enum mympd_cmd_ids cmd_id = MYMPD_API_MOUNT_URLHANDLER_LIST;
    bool rc = mpd_send_command(partition_state->conn, "urlhandlers", NULL);
    if (mympd_check_rc_error_and_recover_respond(partition_state, &buffer, cmd_id, request_id, rc, "urlhandlers") == false) {
        return buffer;
    }

    buffer = jsonrpc_respond_start(buffer, cmd_id, request_id);
    buffer = sdscat(buffer, "\"data\":[");
    long entity_count = 0;
    struct mpd_pair *pair;
    while ((pair = mpd_recv_pair(partition_state->conn)) != NULL) {
        if (entity_count++) {
            buffer = sdscatlen(buffer, ",", 1);
        }
        buffer = sds_catjson(buffer, pair->value, strlen(pair->value));
        mpd_return_pair(partition_state->conn, pair);
    }

    buffer = sdscatlen(buffer, "],", 2);
    buffer = tojson_long(buffer, "totalEntities", entity_count, true);
    buffer = tojson_long(buffer, "returnedEntities", entity_count, false);
    buffer = jsonrpc_respond_end(buffer);

    mpd_response_finish(partition_state->conn);
    if (mympd_check_error_and_recover_respond(partition_state, &buffer, cmd_id, request_id) == false) {
        return buffer;
    }

    return buffer;
}

sds mympd_api_mounts_neighbor_list(struct t_partition_state *partition_state, sds buffer, long request_id) {
    enum mympd_cmd_ids cmd_id = MYMPD_API_MOUNT_NEIGHBOR_LIST;
    bool rc = mpd_send_list_neighbors(partition_state->conn);
    if (mympd_check_rc_error_and_recover_respond(partition_state, &buffer, cmd_id, request_id, rc, "mpd_send_list_neighbors") == false) {
        return buffer;
    }

    buffer = jsonrpc_respond_start(buffer, cmd_id, request_id);
    buffer = sdscat(buffer, "\"data\":[");
    long entity_count = 0;
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
    buffer = tojson_long(buffer, "totalEntities", entity_count, true);
    buffer = tojson_long(buffer, "returnedEntities", entity_count, false);
    buffer = jsonrpc_respond_end(buffer);

    mpd_response_finish(partition_state->conn);
    if (mympd_check_error_and_recover_respond(partition_state, &buffer, cmd_id, request_id) == false) {
        return buffer;
    }

    return buffer;
}
