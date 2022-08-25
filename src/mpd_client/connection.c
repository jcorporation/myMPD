/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2022 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include "compile_time.h"
#include "connection.h"

#include "../lib/api.h"
#include "../lib/jsonrpc.h"
#include "../lib/log.h"
#include "../lib/sds_extras.h"
#include "../lib/utility.h"
#include "errorhandler.h"
#include "mpd/binary.h"
#include "mpd/connection.h"

#include <string.h>

/**
 * Connects to mpd and sets initial connection settings
 * @param partition_state pointer to partition state
 * @return true on success else false
 */
bool mpd_client_connect(struct t_partition_state *partition_state) {
    if (strncmp(partition_state->mpd_state->mpd_host, "/", 1) == 0) {
        MYMPD_LOG_NOTICE("\"%s\": Connecting to socket \"%s\"", partition_state->name, partition_state->mpd_state->mpd_host);
    }
    else {
        MYMPD_LOG_NOTICE("\"%s\": Connecting to \"%s:%d\"", partition_state->name, partition_state->mpd_state->mpd_host, partition_state->mpd_state->mpd_port);
    }
    partition_state->conn = mpd_connection_new(partition_state->mpd_state->mpd_host, partition_state->mpd_state->mpd_port, partition_state->mpd_state->mpd_timeout);
    if (partition_state->conn == NULL) {
        MYMPD_LOG_ERROR("\"%s\": Connection failed: out-of-memory", partition_state->name);
        partition_state->conn_state = MPD_FAILURE;
        mpd_connection_free(partition_state->conn);
        partition_state->conn = NULL;
        sds buffer = jsonrpc_event(sdsempty(), JSONRPC_EVENT_MPD_DISCONNECTED);
        ws_notify(buffer, partition_state->name);
        FREE_SDS(buffer);
        return false;
    }
    if (mpd_connection_get_error(partition_state->conn) != MPD_ERROR_SUCCESS) {
        MYMPD_LOG_ERROR("\"%s\": Connection: %s", partition_state->name, mpd_connection_get_error_message(partition_state->conn));
        partition_state->conn_state = MPD_FAILURE;
        sds buffer = jsonrpc_notify_phrase(sdsempty(), JSONRPC_FACILITY_MPD,
            JSONRPC_SEVERITY_ERROR, "MPD connection error: %{error}", 2,
            "error", mpd_connection_get_error_message(partition_state->conn));
        ws_notify(buffer, partition_state->name);
        FREE_SDS(buffer);
        return false;
    }
    if (sdslen(partition_state->mpd_state->mpd_pass) > 0) {
        MYMPD_LOG_DEBUG("\"%s\": Password set, authenticating to MPD", partition_state->name);
        if (mpd_run_password(partition_state->conn, partition_state->mpd_state->mpd_pass) == false) {
            MYMPD_LOG_ERROR("\"%s\": MPD connection: %s", partition_state->name, mpd_connection_get_error_message(partition_state->conn));
            partition_state->conn_state = MPD_FAILURE;
            sds buffer = jsonrpc_notify_phrase(sdsempty(), JSONRPC_FACILITY_MPD,
                JSONRPC_SEVERITY_ERROR, "MPD connection error: %{error}", 2,
                "error", mpd_connection_get_error_message(partition_state->conn));
            ws_notify(buffer, partition_state->name);
            FREE_SDS(buffer);
            return false;
        }
        MYMPD_LOG_INFO("\"%s\": Successfully authenticated to MPD", partition_state->name);
    }
    else {
        MYMPD_LOG_DEBUG("\"%s\": No password set", partition_state->name);
    }

    MYMPD_LOG_NOTICE("\"%s\": Connected to MPD", partition_state->name);
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
 * @return true on success, else false
 */
bool mpd_client_set_keepalive(struct t_partition_state *partition_state) {
    if (partition_state->mpd_state->mpd_keepalive == true) {
        MYMPD_LOG_INFO("\"%s\": Enabling keepalive", partition_state->name);
    }
    else {
        MYMPD_LOG_INFO("\"%s\": Disabling keepalive", partition_state->name);
    }
    bool rc = mpd_connection_set_keepalive(partition_state->conn, partition_state->mpd_state->mpd_keepalive);
    return mympd_check_rc_error_and_recover(partition_state, rc, "mpd_connection_set_keepalive");
}

/**
 * Sets the mpd timeout
 * @param partition_state pointer to partition state
 * @return true on success, else false
 */
bool mpd_client_set_timeout(struct t_partition_state *partition_state) {
    MYMPD_LOG_INFO("\"%s\": Setting timeout to %u", partition_state->name, partition_state->mpd_state->mpd_timeout);
    mpd_connection_set_timeout(partition_state->conn, partition_state->mpd_state->mpd_timeout);
    return mympd_check_error_and_recover(partition_state);
}

/**
 * Sets the binary limit
 * @param partition_state pointer to partition state
 * @return true on success, else false
 */
bool mpd_client_set_binarylimit(struct t_partition_state *partition_state) {
    bool rc = true;
    if (partition_state->mpd_state->feat_binarylimit == true) {
        MYMPD_LOG_INFO("\"%s\": Setting binarylimit to %u", partition_state->name, partition_state->mpd_state->mpd_binarylimit);
        rc = mpd_run_binarylimit(partition_state->conn, partition_state->mpd_state->mpd_binarylimit);
        sds message = sdsempty();
        if (mympd_check_rc_error_and_recover_notify(partition_state, &message, rc, "mpd_run_binarylimit") == false) {
            ws_notify(message, partition_state->name);
        }
        FREE_SDS(message);
    }
    return rc;
}

/**
 * Sets mpd connection options binarylimit, keepalive and timeout
 * @param partition_state pointer to partition state
 * @return true on success, else false
 */
bool mpd_client_set_connection_options(struct t_partition_state *partition_state) {
    return mpd_client_set_binarylimit(partition_state) &&
        mpd_client_set_keepalive(partition_state) &&
        mpd_client_set_timeout(partition_state);
}

/**
 * Disconnects from MPD
 * @param partition_state pointer to partition state
 * @param new_conn_state new connection state
 */
void mpd_client_disconnect(struct t_partition_state *partition_state, enum mpd_conn_states new_conn_state) {
    if (partition_state->conn != NULL) {
        MYMPD_LOG_INFO("\"%s\": Disconnecting from mpd", partition_state->name);
        mpd_connection_free(partition_state->conn);
    }
    partition_state->conn = NULL;
    partition_state->conn_state = new_conn_state;
}

/**
 * Disconnects all MPD partitions
 * @param mympd_state pointer to central myMPD state
 * @param new_conn_state new connection state
 */
void mpd_client_disconnect_all(struct t_mympd_state *mympd_state, enum mpd_conn_states new_conn_state) {
    struct t_partition_state *partition_state = mympd_state->partition_state;
    while (partition_state != NULL) {
        mpd_client_disconnect(partition_state, new_conn_state);
        partition_state = partition_state->next;
    }
}
