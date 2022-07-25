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

/**
 * Connects to mpd and sets initial connection settings
 * @param partition_state pointer to partition state
 * @return true on success else false
 */
bool mpd_client_connect(struct t_partition_state *partition_state) {
    if (strncmp(partition_state->mpd_shared_state->mpd_host, "/", 1) == 0) {
        MYMPD_LOG_NOTICE("Connecting to socket \"%s\"", partition_state->mpd_shared_state->mpd_host);
    }
    else {
        MYMPD_LOG_NOTICE("Connecting to \"%s:%d\"", partition_state->mpd_shared_state->mpd_host, partition_state->mpd_shared_state->mpd_port);
    }
    partition_state->conn = mpd_connection_new(partition_state->mpd_shared_state->mpd_host, partition_state->mpd_shared_state->mpd_port, partition_state->mpd_shared_state->mpd_timeout);
    if (partition_state->conn == NULL) {
        MYMPD_LOG_ERROR("Connection failed: out-of-memory");
        partition_state->conn_state = MPD_FAILURE;
        mpd_connection_free(partition_state->conn);
        partition_state->conn = NULL;
        sds buffer = jsonrpc_event(sdsempty(), JSONRPC_EVENT_MPD_DISCONNECTED);
        ws_notify(buffer);
        FREE_SDS(buffer);
        return false;
    }
    if (mpd_connection_get_error(partition_state->conn) != MPD_ERROR_SUCCESS) {
        MYMPD_LOG_ERROR("Connection: %s", mpd_connection_get_error_message(partition_state->conn));
        partition_state->conn_state = MPD_FAILURE;
        sds buffer = jsonrpc_notify_phrase(sdsempty(), JSONRPC_FACILITY_MPD,
            JSONRPC_SEVERITY_ERROR, "MPD connection error: %{error}", 2,
            "error", mpd_connection_get_error_message(partition_state->conn));
        ws_notify(buffer);
        FREE_SDS(buffer);
        return false;
    }
    if (sdslen(partition_state->mpd_shared_state->mpd_pass) > 0) {
        MYMPD_LOG_DEBUG("Password set, authenticating to MPD");
        if (mpd_run_password(partition_state->conn, partition_state->mpd_shared_state->mpd_pass) == false) {
            MYMPD_LOG_ERROR("MPD worker connection: %s", mpd_connection_get_error_message(partition_state->conn));
            partition_state->conn_state = MPD_FAILURE;
            sds buffer = jsonrpc_notify_phrase(sdsempty(), JSONRPC_FACILITY_MPD,
                JSONRPC_SEVERITY_ERROR, "MPD connection error: %{error}", 2,
                "error", mpd_connection_get_error_message(partition_state->conn));
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
    partition_state->conn_state = MPD_CONNECTED;
    //set keepalive
    mpd_client_set_keepalive(partition_state);
    //set binary limit
    mpd_client_set_binarylimit(partition_state);
    //reset reconnection intervals
    partition_state->reconnect_interval = 0;
    partition_state->reconnect_time = 0;
    return true;
}

/**
 * Sets the tcp keepalive
 * @param partition_state pointer to partition state
 * @return true on success else false
 */
bool mpd_client_set_keepalive(struct t_partition_state *partition_state) {
    bool rc = mpd_connection_set_keepalive(partition_state->conn, partition_state->mpd_shared_state->mpd_keepalive);
    return mympd_check_rc_error_and_recover(partition_state, rc, "mpd_connection_set_keepalive");
}

/**
 * Sets the binary limit
 * @param partition_state pointer to partition state
 * @return true on success else false
 */
bool mpd_client_set_binarylimit(struct t_partition_state *partition_state) {
    bool rc = true;
    if (partition_state->mpd_shared_state->feat_mpd_binarylimit == true) {
        MYMPD_LOG_INFO("Setting binarylimit to %u", partition_state->mpd_shared_state->mpd_binarylimit);
        rc = mpd_run_binarylimit(partition_state->conn, partition_state->mpd_shared_state->mpd_binarylimit);
        sds message = sdsempty();
        if (mympd_check_rc_error_and_recover_notify(partition_state, &message, rc, "mpd_run_binarylimit") == false) {
            ws_notify(message);
        }
        FREE_SDS(message);
    }
    return rc;
}

/**
 * Disconnects from MPD
 * @param partition_state pointer to partition state
 */
void mpd_client_disconnect(struct t_partition_state *partition_state) {
    if (partition_state->conn != NULL) {
        mpd_connection_free(partition_state->conn);
    }
    partition_state->conn = NULL;
}
