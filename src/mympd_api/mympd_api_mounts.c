/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2022 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include "mympd_config_defs.h"
#include "mympd_api_mounts.h"

#include "../lib/jsonrpc.h"
#include "../lib/sds_extras.h"
#include "../mpd_shared.h"

#include <string.h>

//public functions
sds mympd_api_mounts_list(struct t_mympd_state *mympd_state, sds buffer, sds method, long request_id) {
    bool rc = mpd_send_list_mounts(mympd_state->mpd_state->conn);
    if (check_rc_error_and_recover(mympd_state->mpd_state, &buffer, method, request_id, false, rc, "mpd_send_list_mounts") == false) {
        return buffer;
    }

    buffer = jsonrpc_result_start(buffer, method, request_id);
    buffer = sdscat(buffer, "\"data\":[");
    unsigned entity_count = 0;
    struct mpd_mount *mount;
    while ((mount = mpd_recv_mount(mympd_state->mpd_state->conn)) != NULL) {
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
    buffer = jsonrpc_result_end(buffer);

    mpd_response_finish(mympd_state->mpd_state->conn);
    if (check_error_and_recover2(mympd_state->mpd_state, &buffer, method, request_id, false) == false) {
        return buffer;
    }

    return buffer;
}

sds mympd_api_mounts_urlhandler_list(struct t_mympd_state *mympd_state, sds buffer, sds method, long request_id) {
    bool rc = mpd_send_command(mympd_state->mpd_state->conn, "urlhandlers", NULL);
    if (check_rc_error_and_recover(mympd_state->mpd_state, &buffer, method, request_id, false, rc, "urlhandlers") == false) {
        return buffer;
    }

    buffer = jsonrpc_result_start(buffer, method, request_id);
    buffer = sdscat(buffer, "\"data\":[");
    unsigned entity_count = 0;
    struct mpd_pair *pair;
    while ((pair = mpd_recv_pair(mympd_state->mpd_state->conn)) != NULL) {
        if (entity_count++) {
            buffer = sdscatlen(buffer, ",", 1);
        }
        buffer = sds_catjson(buffer, pair->value, strlen(pair->value));
        mpd_return_pair(mympd_state->mpd_state->conn, pair);
    }

    buffer = sdscatlen(buffer, "],", 2);
    buffer = tojson_long(buffer, "totalEntities", entity_count, true);
    buffer = tojson_long(buffer, "returnedEntities", entity_count, false);
    buffer = jsonrpc_result_end(buffer);

    mpd_response_finish(mympd_state->mpd_state->conn);
    if (check_error_and_recover2(mympd_state->mpd_state, &buffer, method, request_id, false) == false) {
        return buffer;
    }

    return buffer;
}

sds mympd_api_mounts_neighbor_list(struct t_mympd_state *mympd_state, sds buffer, sds method, long request_id) {
    bool rc = mpd_send_list_neighbors(mympd_state->mpd_state->conn);
    if (check_rc_error_and_recover(mympd_state->mpd_state, &buffer, method, request_id, false, rc, "mpd_send_list_neighbors") == false) {
        return buffer;
    }

    buffer = jsonrpc_result_start(buffer, method, request_id);
    buffer = sdscat(buffer, "\"data\":[");
    unsigned entity_count = 0;
    struct mpd_neighbor *neighbor;
    while ((neighbor = mpd_recv_neighbor(mympd_state->mpd_state->conn)) != NULL) {
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
    buffer = jsonrpc_result_end(buffer);

    mpd_response_finish(mympd_state->mpd_state->conn);
    if (check_error_and_recover2(mympd_state->mpd_state, &buffer, method, request_id, false) == false) {
        return buffer;
    }

    return buffer;
}
