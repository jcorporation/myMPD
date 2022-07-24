/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2022 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include "mympd_config_defs.h"
#include "mympd_api_stats.h"

#include "../lib/jsonrpc.h"
#include "../lib/log.h"
#include "../lib/sds_extras.h"
#include "../mpd_client/mpd_client_errorhandler.h"

sds mympd_api_stats_get(struct t_mympd_state *mympd_state, sds buffer, long request_id) {
    enum mympd_cmd_ids cmd_id = MYMPD_API_DATABASE_STATS;
    struct mpd_stats *stats = mpd_run_stats(mympd_state->mpd_state->conn);
    if (stats == NULL) {
        mympd_check_error_and_recover_respond(mympd_state->mpd_state, &buffer, cmd_id, request_id);
        return buffer;
    }

    const unsigned *version = mpd_connection_get_server_version(mympd_state->mpd_state->conn);
    sds mpd_protocol_version = sdscatfmt(sdsempty(),"%u.%u.%u", version[0], version[1], version[2]);

    buffer = jsonrpc_respond_start(buffer, cmd_id, request_id);
    buffer = tojson_uint(buffer, "artists", mpd_stats_get_number_of_artists(stats), true);
    buffer = tojson_uint(buffer, "albums", mpd_stats_get_number_of_albums(stats), true);
    buffer = tojson_uint(buffer, "songs", mpd_stats_get_number_of_songs(stats), true);
    buffer = tojson_ulong(buffer, "playtime", mpd_stats_get_play_time(stats), true);
    buffer = tojson_ulong(buffer, "uptime", mpd_stats_get_uptime(stats), true);
    buffer = tojson_llong(buffer, "myMPDuptime", (long long)(time(NULL) - mympd_state->config->startup_time), true);
    buffer = tojson_ulong(buffer, "dbUpdated", mpd_stats_get_db_update_time(stats), true);
    buffer = tojson_ulong(buffer, "dbPlaytime", mpd_stats_get_db_play_time(stats), true);
    buffer = tojson_char(buffer, "mympdVersion", MYMPD_VERSION, true);
    buffer = tojson_char(buffer, "mpdProtocolVersion", mpd_protocol_version, false);
    buffer = jsonrpc_respond_end(buffer);

    FREE_SDS(mpd_protocol_version);
    mpd_stats_free(stats);

    return buffer;
}
