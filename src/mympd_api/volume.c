/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2022 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include "compile_time.h"
#include "src/mympd_api/volume.h"

#include "dist/libmympdclient/include/mpd/client.h"
#include "src/lib/api.h"
#include "src/lib/jsonrpc.h"
#include "src/lib/log.h"
#include "src/mpd_client/errorhandler.h"
#include "src/mpd_client/volume.h"

/**
 * Sets an absolute volume level
 * @param partition_state pointer to partition state
 * @param buffer already allocated sds string to append the response
 * @param cmd_id jsonrpc method
 * @param request_id jsonrpc request id
 * @param volume volume percent to set
 * @return pointer to buffer
 */
sds mympd_api_volume_set(struct t_partition_state *partition_state, sds buffer, enum mympd_cmd_ids cmd_id, long request_id, unsigned volume) {
    if (volume > partition_state->mympd_state->volume_max ||
        volume < partition_state->mympd_state->volume_min)
    {
        //enforce volume range limit
        buffer = jsonrpc_respond_message(buffer, cmd_id, request_id,
            JSONRPC_FACILITY_PLAYER, JSONRPC_SEVERITY_ERROR, "Invalid volume level");
        MYMPD_LOG_WARN("New volume(%u) out of range: %u-%u",
            volume, partition_state->mympd_state->volume_min, partition_state->mympd_state->volume_max);
    }
    else {
        bool rc = mpd_run_set_volume(partition_state->conn, volume);
        bool result;
        buffer = mympd_respond_with_error_or_ok(partition_state, buffer, cmd_id, request_id, rc, "mpd_run_set_volume", &result);
    }
    return buffer;
}

/**
 * Changes the volume by relative_volume
 * @param partition_state pointer to partition state
 * @param buffer already allocated sds string to append the response
 * @param request_id jsonrpc request id
 * @param relative_volume the relative volume between -100 and 100
 * @return pointer to buffer
 */
sds mympd_api_volume_change(struct t_partition_state *partition_state, sds buffer, long request_id, int relative_volume) {
    //calculate absolute volume
    int curVolume = mpd_client_get_volume(partition_state);
    int newVolume = curVolume + relative_volume;

    //enforce volume range limit
    unsigned newVol = newVolume < (int)partition_state->mympd_state->volume_min
        ? partition_state->mympd_state->volume_min
        : newVolume > (int)partition_state->mympd_state->volume_max
            ? partition_state->mympd_state->volume_max
            : (unsigned)newVolume;

    return mympd_api_volume_set(partition_state, buffer, MYMPD_API_PLAYER_VOLUME_CHANGE, request_id, newVol);
}
