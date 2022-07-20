/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2022 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include "mympd_config_defs.h"
#include "mpd_client_connection.h"

#include "../lib/api.h"
#include "../lib/jsonrpc.h"
#include "../lib/log.h"
#include "../lib/sds_extras.h"
#include "../lib/utility.h"
#include "mpd_client_errorhandler.h"

#include <string.h>

bool mpd_client_connect(struct t_mpd_state *mpd_state) {
    if (strncmp(mpd_state->mpd_host, "/", 1) == 0) {
        MYMPD_LOG_NOTICE("Connecting to socket \"%s\"", mpd_state->mpd_host);
    }
    else {
        MYMPD_LOG_NOTICE("Connecting to \"%s:%d\"", mpd_state->mpd_host, mpd_state->mpd_port);
    }
    mpd_state->conn = mpd_connection_new(mpd_state->mpd_host, mpd_state->mpd_port, mpd_state->mpd_timeout);
    if (mpd_state->conn == NULL) {
        MYMPD_LOG_ERROR("Connection failed: out-of-memory");
        mpd_state->conn_state = MPD_FAILURE;
        mpd_connection_free(mpd_state->conn);
        mpd_state->conn = NULL;
        sds buffer = jsonrpc_event(sdsempty(), JSONRPC_EVENT_MPD_DISCONNECTED);
        ws_notify(buffer);
        FREE_SDS(buffer);
        return false;
    }
    if (mpd_connection_get_error(mpd_state->conn) != MPD_ERROR_SUCCESS) {
        MYMPD_LOG_ERROR("Connection: %s", mpd_connection_get_error_message(mpd_state->conn));
        mpd_state->conn_state = MPD_FAILURE;
        sds buffer = jsonrpc_notify_phrase(sdsempty(), JSONRPC_FACILITY_MPD, JSONRPC_SEVERITY_ERROR,
            "MPD connection error: %{error}", 2,
            "error", mpd_connection_get_error_message(mpd_state->conn));
        ws_notify(buffer);
        FREE_SDS(buffer);
        return false;
    }
    if (sdslen(mpd_state->mpd_pass) > 0) {
        MYMPD_LOG_DEBUG("Password set, authenticating to MPD");
        if (mpd_run_password(mpd_state->conn, mpd_state->mpd_pass) == false) {
            MYMPD_LOG_ERROR("MPD worker connection: %s", mpd_connection_get_error_message(mpd_state->conn));
            mpd_state->conn_state = MPD_FAILURE;
            sds buffer = jsonrpc_notify_phrase(sdsempty(), JSONRPC_FACILITY_MPD, JSONRPC_SEVERITY_ERROR,
                "MPD connection error: %{error}", 2,
                "error", mpd_connection_get_error_message(mpd_state->conn));
            ws_notify(buffer);
            FREE_SDS(buffer);
            return false;
        }
        MYMPD_LOG_INFO("Successfully authenticated to MPD");
    }
    else {
        MYMPD_LOG_DEBUG("No password set");
    }

    MYMPD_LOG_NOTICE("Connected to MPD");
    mpd_state->conn_state = MPD_CONNECTED;
    //set keepalive
    mpd_client_set_keepalive(mpd_state);

    return true;
}

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
