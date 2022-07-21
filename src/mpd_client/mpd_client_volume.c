/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2022 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include "mympd_config_defs.h"
#include "mpd_client_volume.h"

#include "mpd_client_errorhandler.h"

//returns the mpd volume, -1 if volume control is disabled
int mpd_client_get_volume(struct t_mpd_state *mpd_state) {
    int volume = -1;
    if (mpd_connection_cmp_server_version(mpd_state->conn, 0, 23, 0) >= 0) {
        volume = mpd_run_get_volume(mpd_state->conn);
    }
    else {
        struct mpd_status *status = mpd_run_status(mpd_state->conn);
        if (status == NULL) {
            mympd_check_error_and_recover(mpd_state);
            return -1;
        }
        volume = mpd_status_get_volume(status);
        mpd_status_free(status);
    }
    mpd_response_finish(mpd_state->conn);
    mympd_check_error_and_recover(mpd_state);
    return volume;
}
