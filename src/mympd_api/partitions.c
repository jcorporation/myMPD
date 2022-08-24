/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2022 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include "compile_time.h"
#include "partitions.h"

#include "../lib/jsonrpc.h"
#include "../lib/log.h"
#include "../lib/mympd_state.h"
#include "../mpd_client/errorhandler.h"
#include "../mpd_client/partitions.h"
#include "src/lib/api.h"
#include "src/mpd_client/connection.h"

#include <string.h>

/**
 * Lists partitions
 * @param partition_state pointer to partition state
 * @param buffer already allocated sds string to append the response
 * @param request_id jsonrpc request id
 * @return pointer to buffer
 */
sds mympd_api_partition_list(struct t_partition_state *partition_state, sds buffer, long request_id) {
    enum mympd_cmd_ids cmd_id = MYMPD_API_PARTITION_LIST;
    bool rc = mpd_send_listpartitions(partition_state->conn);
    if (mympd_check_rc_error_and_recover_respond(partition_state, &buffer, cmd_id, request_id, rc, "mpd_send_listpartitions") == false) {
        return buffer;
    }

    buffer = jsonrpc_respond_start(buffer, cmd_id, request_id);
    buffer = sdscat(buffer, "\"data\":[");
    long entity_count = 0;
    struct mpd_pair *partition;
    while ((partition = mpd_recv_partition_pair(partition_state->conn)) != NULL) {
        if (entity_count++) {
            buffer = sdscatlen(buffer, ",", 1);
        }
        buffer = sdscatlen(buffer, "{", 1);
        buffer = tojson_char(buffer, "name", partition->value, false);
        buffer = sdscatlen(buffer, "}", 1);
        mpd_return_pair(partition_state->conn, partition);
    }

    buffer = sdscatlen(buffer, "],", 2);
    buffer = tojson_long(buffer, "totalEntities", entity_count, true);
    buffer = tojson_long(buffer, "returnedEntities", entity_count, false);
    buffer = jsonrpc_end(buffer);

    mpd_response_finish(partition_state->conn);
    if (mympd_check_error_and_recover_respond(partition_state, &buffer, cmd_id, request_id) == false) {
        return buffer;
    }

    return buffer;
}

/**
 * Disconnects and removes a partition.
 * Moves partitions outputs to default partition and disconnects the myMPD partition connection.
 * @param partition_state pointer to partition state
 * @param buffer already allocated sds string to append the response
 * @param request_id jsonrpc request id
 * @param partition partition to remove
 * @return pointer to buffer
 */
sds mympd_api_partition_rm(struct t_partition_state *partition_state, sds buffer, long request_id, sds partition) {
    enum mympd_cmd_ids cmd_id = MYMPD_API_PARTITION_RM;
    struct t_partition_state *partition_to_remove = partitions_get_by_name(partition_state->mympd_state, partition);
    if (partition_to_remove == NULL) {
        buffer = jsonrpc_respond_message(buffer, cmd_id, request_id, JSONRPC_FACILITY_MPD, JSONRPC_SEVERITY_ERROR, "Partition not found");
        return buffer;
    }
    //disconnect partition
    mpd_client_disconnect(partition_to_remove, MPD_DISCONNECTED);
    //delete the partition
    bool rc = mpd_run_delete_partition(partition_state->conn, partition);
    bool result = false;
    buffer = mympd_respond_with_error_or_ok(partition_state, buffer, cmd_id, request_id, rc, "mpd_run_delete_partition", &result);
    if (result == true) {
        //partition could not be deleted
        partition_to_remove->conn_state = MPD_REMOVED;
    }
    return buffer;
}
