/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2025 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

/*! \file
 * \brief MPD volume wrapper
 */

#include "compile_time.h"
#include "src/mpd_client/volume.h"

#include "src/mpd_client/errorhandler.h"

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
        if (status != NULL) {
            volume = mpd_status_get_volume(status);
            mpd_status_free(status);
        }
        mpd_response_finish(partition_state->conn);
    }
    if (mympd_check_error_and_recover(partition_state, NULL, "mpd_run_status") == false) {
        volume = -1;
    }
    return volume;
}
