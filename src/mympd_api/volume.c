/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2022 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include "compile_time.h"
#include "volume.h"

#include "../lib/api.h"
#include "../lib/jsonrpc.h"
#include "../lib/log.h"
#include "../mpd_client/errorhandler.h"
#include "../mpd_client/volume.h"

#include <mpd/client.h>

/**
 * Sets an absolute volume level
 * @param partition_state pointer to partition state
 * @param buffer already allocated sds string to append the response
 * @param request_id jsonrpc request id
 * @param volume volume percent to set
 * @return pointer to buffer
 */
sds mympd_api_volume_set(struct t_partition_state *partition_state, sds buffer, long request_id, unsigned volume) {
    enum mympd_cmd_ids cmd_id = MYMPD_API_PLAYER_VOLUME_SET;
    if (volume > partition_state->mympd_state->volume_max ||
        volume < partition_state->mympd_state->volume_min)
    {
        //enforce volume range limit
        buffer = jsonrpc_respond_message(buffer, cmd_id, request_id,
            JSONRPC_FACILITY_PLAYER, JSONRPC_SEVERITY_ERROR, "Invalid volume level");
    }
    else {
        bool rc = mpd_run_set_volume(partition_state->conn, volume);
        bool result;
        buffer = mympd_respond_with_error_or_ok(partition_state, buffer, cmd_id, request_id, rc, "mpd_run_set_volume", &result);
    }
    return buffer;
}

/**
 * 
 * @param partition_state pointer to partition state
 * @param buffer already allocated sds string to append the response
 * @param request_id jsonrpc request id
 * @param relative_volume the relative volume between -100 and 100
 * @return pointer to buffer
 */
sds mympd_api_volume_change(struct t_partition_state *partition_state, sds buffer, long request_id, int relative_volume) {
    enum mympd_cmd_ids cmd_id = MYMPD_API_PLAYER_VOLUME_CHANGE;

    if (partition_state->mympd_state->volume_min > 0 ||
        partition_state->mympd_state->volume_max < 100)
    {
        //enforce volume range limit
        int curVolume = mpd_client_get_volume(partition_state);
        int newVolume = curVolume + relative_volume;
        if (newVolume < (int)partition_state->mympd_state->volume_min) {
            relative_volume = (int)partition_state->mympd_state->volume_min - curVolume;
            MYMPD_LOG_WARN("New volume(%d) out of range: %u-%u",
                newVolume, partition_state->mympd_state->volume_min, partition_state->mympd_state->volume_max);
        }
        else if (newVolume > (int)partition_state->mympd_state->volume_max) {
            relative_volume = (int)partition_state->mympd_state->volume_max - curVolume;
            MYMPD_LOG_WARN("New volume(%d) out of range: %u-%u",
                newVolume, partition_state->mympd_state->volume_min, partition_state->mympd_state->volume_max);
        }
    }
    if (relative_volume == 0) {
        buffer = jsonrpc_respond_ok(buffer, cmd_id, request_id, JSONRPC_FACILITY_PLAYER);
    }
    else {
        bool rc = mpd_run_change_volume(partition_state->conn, relative_volume);
        bool result;
        buffer = mympd_respond_with_error_or_ok(partition_state, buffer, cmd_id, request_id, rc, "mpd_run_change_volume", &result);
    }
    return buffer;
}
