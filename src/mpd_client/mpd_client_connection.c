/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2022 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include "mympd_config_defs.h"
#include "mpd_client_connection.h"

#include "../lib/api.h"
#include "../lib/log.h"
#include "../lib/sds_extras.h"
#include "../lib/utility.h"
#include "mpd_client_errorhandler.h"

bool mpd_client_set_keepalive(struct t_mpd_state *mpd_state) {
    bool rc = mpd_connection_set_keepalive(mpd_state->conn, mpd_state->mpd_keepalive);
    return check_rc_error_and_recover(mpd_state, NULL, NULL, 0, false, rc, "mpd_connection_set_keepalive");
}

bool mpd_client_set_binarylimit(struct t_mpd_state *mpd_state) {
    bool rc = true;
    if (mpd_state->feat_mpd_binarylimit == true) {
        MYMPD_LOG_INFO("Setting binarylimit to %u", mpd_state->mpd_binarylimit);
        rc = mpd_run_binarylimit(mpd_state->conn, mpd_state->mpd_binarylimit);
        sds message = sdsempty();
        rc = check_rc_error_and_recover(mpd_state, &message, NULL, 0, true, rc, "mpd_run_binarylimit");
        if (sdslen(message) > 0) {
            ws_notify(message);
            rc = false;
        }
        FREE_SDS(message);
    }
    return rc;
}

void mpd_client_disconnect(struct t_mpd_state *mpd_state) {
    mpd_state->conn_state = MPD_DISCONNECT;
    if (mpd_state->conn != NULL) {
        mpd_connection_free(mpd_state->conn);
    }
}
