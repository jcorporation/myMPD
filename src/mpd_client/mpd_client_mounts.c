/*
 SPDX-License-Identifier: GPL-2.0-or-later
 myMPD (c) 2018-2020 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <mpd/client.h>

#include "../../dist/src/sds/sds.h"
#include "../sds_extras.h"
#include "../api.h"
#include "../log.h"
#include "../list.h"
#include "config_defs.h"
#include "../utility.h"
#include "mpd_client_utility.h"
#include "mpd_client_mounts.h"

//public functions
sds mpd_client_put_mounts(t_mpd_state *mpd_state, sds buffer, sds method, int request_id) {
    if (mpd_send_list_mounts(mpd_state->conn) == false) {
        buffer = check_error_and_recover(mpd_state, buffer, method, request_id);
        return buffer;
    }
        
    buffer = jsonrpc_start_result(buffer, method, request_id);
    buffer = sdscat(buffer, ",\"data\":[");
    unsigned entity_count = 0;
    struct mpd_mount *mount;
    while ((mount = mpd_recv_mount(mpd_state->conn)) != NULL) {
        const char *uri = mpd_mount_get_uri(mount);
        const char *storage = mpd_mount_get_storage(mount);
        if (uri != NULL && storage != NULL) {
            if (entity_count++) {
                buffer = sdscat(buffer, ",");
            }
            buffer = sdscat(buffer, "{");
            buffer = tojson_char(buffer, "mountPoint", uri, true);
            buffer = tojson_char(buffer, "mountUrl", storage, false);
            buffer = sdscat(buffer, "}");
        }
        mpd_mount_free(mount);
    }

    buffer = sdscat(buffer, "],");
    buffer = tojson_long(buffer, "totalEntities", entity_count, true);
    buffer = tojson_long(buffer, "returnedEntities", entity_count, false);
    buffer = jsonrpc_end_result(buffer);
    
    if (check_error_and_recover2(mpd_state, &buffer, method, request_id, false) == false) {
        return buffer;
    }
    
    return buffer;
}

sds mpd_client_put_urlhandlers(t_mpd_state *mpd_state, sds buffer, sds method, int request_id) {
    if (mpd_send_command(mpd_state->conn, "urlhandlers", NULL) == false) {
        buffer = check_error_and_recover(mpd_state, buffer, method, request_id);
        return buffer;
    }
        
    buffer = jsonrpc_start_result(buffer, method, request_id);
    buffer = sdscat(buffer, ",\"data\":[");
    unsigned entity_count = 0;
    struct mpd_pair *pair;
    while ((pair = mpd_recv_pair(mpd_state->conn)) != NULL) {
        if (entity_count++) {
            buffer = sdscat(buffer, ",");
        }
        buffer = sdscatjson(buffer, pair->value, strlen(pair->value));
        mpd_return_pair(mpd_state->conn, pair);
    }

    buffer = sdscat(buffer, "],");
    buffer = tojson_long(buffer, "totalEntities", entity_count, true);
    buffer = tojson_long(buffer, "returnedEntities", entity_count, false);
    buffer = jsonrpc_end_result(buffer);
    
    if (check_error_and_recover2(mpd_state, &buffer, method, request_id, false) == false) {
        return buffer;
    }

    return buffer;
}

sds mpd_client_put_neighbors(t_mpd_state *mpd_state, sds buffer, sds method, int request_id) {
    if (mpd_send_list_neighbors(mpd_state->conn) == false) {
        buffer = check_error_and_recover(mpd_state, buffer, method, request_id);
        return buffer;
    }
        
    buffer = jsonrpc_start_result(buffer, method, request_id);
    buffer = sdscat(buffer, ",\"data\":[");
    unsigned entity_count = 0;
    struct mpd_neighbor *neighbor;
    while ((neighbor = mpd_recv_neighbor(mpd_state->conn)) != NULL) {
        if (entity_count++) {
            buffer = sdscat(buffer, ",");
        }
        buffer = sdscat(buffer, "{");
        buffer = tojson_char(buffer, "uri", mpd_neighbor_get_uri(neighbor), true);
        buffer = tojson_char(buffer, "displayName", mpd_neighbor_get_display_name(neighbor), false);
        buffer = sdscat(buffer, "}");
        mpd_neighbor_free(neighbor);
    }

    buffer = sdscat(buffer, "],");
    buffer = tojson_long(buffer, "totalEntities", entity_count, true);
    buffer = tojson_long(buffer, "returnedEntities", entity_count, false);
    buffer = jsonrpc_end_result(buffer);
    
    if (check_error_and_recover2(mpd_state, &buffer, method, request_id, false) == false) {
        return buffer;
    }
    
    return buffer;
}
