/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2025 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

/*! \file
 * \brief myMPD stats API
 */

#include "compile_time.h"
#include "src/mympd_api/stats.h"

#include "src/lib/jsonrpc.h"
#include "src/lib/sds_extras.h"
#include "src/lib/utility.h"
#include "src/mpd_client/errorhandler.h"

/**
 * Get mpd statistics
 * @param partition_state pointer to partition state
 * @param buffer already allocated sds string to append the response
 * @param request_id jsonrpc request id
 * @return pointer to buffer
 */
sds mympd_api_stats_get(struct t_partition_state *partition_state, sds buffer, unsigned request_id) {
    enum mympd_cmd_ids cmd_id = MYMPD_API_STATS;
    struct mpd_stats *stats = mpd_run_stats(partition_state->conn);
    if (stats != NULL) {
        const unsigned *version = mpd_connection_get_server_version(partition_state->conn);
        sds mpd_protocol_version = sdscatfmt(sdsempty(),"%u.%u.%u", version[0], version[1], version[2]);
        sds mympd_uri = sdsnew("mympd://");
        mympd_uri = resolv_mympd_uri(mympd_uri, partition_state->mpd_state->mpd_host, partition_state->config, true);

        buffer = jsonrpc_respond_start(buffer, cmd_id, request_id);
        buffer = tojson_uint(buffer, "artists", mpd_stats_get_number_of_artists(stats), true);
        buffer = tojson_uint(buffer, "albums", mpd_stats_get_number_of_albums(stats), true);
        buffer = tojson_uint(buffer, "songs", mpd_stats_get_number_of_songs(stats), true);
        buffer = tojson_uint64(buffer, "playtime", mpd_stats_get_play_time(stats), true);
        buffer = tojson_uint64(buffer, "uptime", mpd_stats_get_uptime(stats), true);
        buffer = tojson_time(buffer, "myMPDuptime", (time(NULL) - partition_state->config->startup_time), true);
        buffer = tojson_uint64(buffer, "dbUpdated", mpd_stats_get_db_update_time(stats), true);
        buffer = tojson_uint64(buffer, "dbPlaytime", mpd_stats_get_db_play_time(stats), true);
        buffer = tojson_char(buffer, "mympdVersion", MYMPD_VERSION, true);
        buffer = tojson_char(buffer, "mpdProtocolVersion", mpd_protocol_version, true);
        buffer = tojson_char(buffer, "myMPDuri", mympd_uri,false);
        buffer = jsonrpc_end(buffer);

        FREE_SDS(mympd_uri);
        FREE_SDS(mpd_protocol_version);
        mpd_stats_free(stats);
    }
    mpd_response_finish(partition_state->conn);
    mympd_check_error_and_recover_respond(partition_state, &buffer, cmd_id, request_id, "mpd_run_stats");
    return buffer;
}
