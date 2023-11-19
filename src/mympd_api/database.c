/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2023 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include "compile_time.h"
#include "src/mympd_api/database.h"

#include "dist/libmympdclient/include/mpd/client.h"
#include "src/lib/jsonrpc.h"
#include "src/mpd_client/errorhandler.h"
#include "src/mympd_api/status.h"

/**
 * Starts mpd database update or rescan.
 * It checks if a database update is already running.
 * @param partition_state pointer to partition state
 * @param buffer pointer to sds string to append the jsonrpc result
 * @param cmd_id jsonrpc method
 * @param request_id mongoose request id
 * @param path path to update
 * @return pointer to buffer
 */
sds mympd_api_database_update(struct t_partition_state *partition_state, sds buffer, enum mympd_cmd_ids cmd_id, unsigned request_id, sds path) {
    unsigned update_id = mympd_api_status_updatedb_id(partition_state);

    if (update_id == UINT_MAX) {
        return jsonrpc_respond_message(buffer, cmd_id, request_id,
                JSONRPC_FACILITY_DATABASE, JSONRPC_SEVERITY_ERROR, "Error getting database update id");
    }
    if (update_id > 0) {
        return jsonrpc_respond_message(buffer, cmd_id, request_id,
                JSONRPC_FACILITY_DATABASE, JSONRPC_SEVERITY_INFO, "Database update already started");
    }

    const char *real_path = sdslen(path) == 0
        ? NULL
        : path;

    bool rc;
    if (cmd_id == MYMPD_API_DATABASE_UPDATE) {
        mpd_run_update(partition_state->conn, real_path);
        return mympd_respond_with_error_or_ok(partition_state, buffer, cmd_id, request_id, "mpd_run_update", &rc);
    }
    mpd_run_rescan(partition_state->conn, real_path);
    return mympd_respond_with_error_or_ok(partition_state, buffer, cmd_id, request_id, "mpd_run_rescan", &rc);
}
