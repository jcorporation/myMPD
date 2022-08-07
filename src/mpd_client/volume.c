/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2022 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include "compile_time.h"
#include "volume.h"

#include "errorhandler.h"

/**
 * Requests the volume from MPD.
 * @param partition_state pointer to partition specific states
 * @return mpd volume or -1 if volume control is disabled
 */
int mpd_client_get_volume(struct t_partition_state *partition_state) {
    int volume = -1;
    if (mpd_connection_cmp_server_version(partition_state->conn, 0, 23, 0) >= 0) {
        volume = mpd_run_get_volume(partition_state->conn);
    }
    else {
        struct mpd_status *status = mpd_run_status(partition_state->conn);
        if (status == NULL) {
            mympd_check_error_and_recover(partition_state);
            return -1;
        }
        volume = mpd_status_get_volume(status);
        mpd_status_free(status);
    }
    mpd_response_finish(partition_state->conn);
    mympd_check_error_and_recover(partition_state);
    return volume;
}
