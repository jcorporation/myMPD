/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2022 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include "mympd_config_defs.h"
#include "mympd_api_partitions.h"

#include "../lib/jsonrpc.h"
#include "../mpd_shared.h"

//public functions
sds mympd_api_partition_list(struct t_mympd_state *mympd_state, sds buffer, sds method, long request_id) {
    bool rc = mpd_send_listpartitions(mympd_state->mpd_state->conn);
    if (check_rc_error_and_recover(mympd_state->mpd_state, &buffer, method, request_id, false, rc, "mpd_send_listpartitions") == false) {
        return buffer;
    }

    buffer = jsonrpc_result_start(buffer, method, request_id);
    buffer = sdscat(buffer, "\"data\":[");
    long entity_count = 0;
    struct mpd_pair *partition;
    while ((partition = mpd_recv_partition_pair(mympd_state->mpd_state->conn)) != NULL) {
        if (entity_count++) {
            buffer = sdscatlen(buffer, ",", 1);
        }
        buffer = sdscatlen(buffer, "{", 1);
        buffer = tojson_char(buffer, "name", partition->value, false);
        buffer = sdscatlen(buffer, "}", 1);
        mpd_return_pair(mympd_state->mpd_state->conn, partition);
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
