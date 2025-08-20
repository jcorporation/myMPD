/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2025 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

/*! \file
 * \brief MPD connection handling
 */

#include "compile_time.h"
#include "src/mympd_client/connection.h"

#include "dist/libmympdclient/include/mpd/client.h"
#include "src/lib/api.h"
#include "src/lib/json/json_rpc.h"
#include "src/lib/log.h"
#include "src/lib/sds_extras.h"
#include "src/mympd_api/requests.h"
#include "src/mympd_client/errorhandler.h"
#include "src/mympd_client/shortcuts.h"
#include "src/mympd_client/tags.h"

/**
 * Connects to mpd and sets initial connection settings
 * @param partition_state pointer to partition state
 * @return true on success, else false
 */
bool mympd_client_connect(struct t_partition_state *partition_state) {
    if (partition_state->mpd_state->mpd_host[0] == '/') {
        MYMPD_LOG_NOTICE(partition_state->name, "Connecting to socket \"%s\"", partition_state->mpd_state->mpd_host);
    }
    else {
        MYMPD_LOG_NOTICE(partition_state->name, "Connecting to \"%s:%d\"", partition_state->mpd_state->mpd_host, partition_state->mpd_state->mpd_port);
    }
    partition_state->conn = mpd_connection_new(partition_state->mpd_state->mpd_host, partition_state->mpd_state->mpd_port, partition_state->mpd_state->mpd_timeout);
    if (partition_state->conn == NULL) {
        MYMPD_LOG_ERROR(partition_state->name, "Connection failed: out-of-memory");
        partition_state->conn_state = MPD_FAILURE;
        sds buffer = jsonrpc_event(sdsempty(), JSONRPC_EVENT_MPD_DISCONNECTED);
        ws_notify(buffer, partition_state->name);
        FREE_SDS(buffer);
        return false;
    }
    if (mpd_connection_get_error(partition_state->conn) != MPD_ERROR_SUCCESS) {
        MYMPD_LOG_ERROR(partition_state->name, "Connection: %s", mpd_connection_get_error_message(partition_state->conn));
        sds buffer = jsonrpc_notify_phrase(sdsempty(), JSONRPC_FACILITY_MPD,
            JSONRPC_SEVERITY_ERROR, "MPD connection error: %{error}", 2,
            "error", mpd_connection_get_error_message(partition_state->conn));
        ws_notify(buffer, partition_state->name);
        FREE_SDS(buffer);
        mpd_connection_free(partition_state->conn);
        partition_state->conn = NULL;
        partition_state->conn_state = MPD_FAILURE;
        return false;
    }
    if (sdslen(partition_state->mpd_state->mpd_pass) > 0) {
        MYMPD_LOG_DEBUG(partition_state->name, "Password set, authenticating to MPD");
        if (mpd_run_password(partition_state->conn, partition_state->mpd_state->mpd_pass) == false) {
            MYMPD_LOG_ERROR(partition_state->name, "MPD connection: %s", mpd_connection_get_error_message(partition_state->conn));
            partition_state->conn_state = MPD_FAILURE;
            sds buffer = jsonrpc_notify_phrase(sdsempty(), JSONRPC_FACILITY_MPD,
                JSONRPC_SEVERITY_ERROR, "MPD connection error: %{error}", 2,
                "error", mpd_connection_get_error_message(partition_state->conn));
            ws_notify(buffer, partition_state->name);
            FREE_SDS(buffer);
            return false;
        }
        MYMPD_LOG_INFO(partition_state->name, "Successfully authenticated to MPD");
    }
    else {
        MYMPD_LOG_DEBUG(partition_state->name, "No password set");
    }

    MYMPD_LOG_NOTICE(partition_state->name, "Connected to MPD");
    partition_state->conn_state = MPD_CONNECTED;
    //set connection options
    mympd_client_set_connection_options(partition_state);
    return true;
}

/**
 * Sets the tcp keepalive
 * @param partition_state pointer to partition state
 * @return true on success, else false
 */
static bool mympd_client_set_keepalive(struct t_partition_state *partition_state) {
    if (partition_state->mpd_state->mpd_keepalive == true) {
        MYMPD_LOG_INFO(partition_state->name, "Enabling keepalive");
    }
    else {
        MYMPD_LOG_INFO(partition_state->name, "Disabling keepalive");
    }
    return mpd_connection_set_keepalive(partition_state->conn, partition_state->mpd_state->mpd_keepalive);
}

/**
 * Sets the mpd timeout
 * @param partition_state pointer to partition state
 * @return true on success, else false
 */
static bool mympd_client_set_timeout(struct t_partition_state *partition_state) {
    MYMPD_LOG_INFO(partition_state->name, "Setting timeout to %u ms", partition_state->mpd_state->mpd_timeout);
    mpd_connection_set_timeout(partition_state->conn, partition_state->mpd_state->mpd_timeout);
    return true;
}

/**
 * Sets mpd connection options binarylimit and protocol features
 * @param partition_state pointer to partition state
 * @return true on success, else false
 */
static bool mympd_client_set_protocol_options(struct t_partition_state *partition_state) {
    if (mpd_command_list_begin(partition_state->conn, false)) {
        if (mpd_connection_cmp_server_version(partition_state->conn, 0, 22, 4) >= 0) {
            MYMPD_LOG_INFO(partition_state->name, "Setting binarylimit to %u kB", partition_state->mpd_state->mpd_binarylimit);
            if (mpd_send_binarylimit(partition_state->conn, partition_state->mpd_state->mpd_binarylimit) == false) {
                mympd_set_mpd_failure(partition_state, "Failure adding command to command list mpd_send_binarylimit");
            }
        }
        if (mpd_connection_cmp_server_version(partition_state->conn, 0, 24, 0) >= 0) {
            MYMPD_LOG_INFO(partition_state->name, "Enabling all protocol features");
            if (mpd_send_all_protocol_features(partition_state->conn) == false) {
                mympd_set_mpd_failure(partition_state, "Failure adding command to command list mpd_send_all_protocol_features");
            }
        }
        if (mpd_connection_cmp_server_version(partition_state->conn, 0, 25, 0) >= 0) {
            if (partition_state->mpd_state->mpd_stringnormalization == true) {
                MYMPD_LOG_INFO(partition_state->name, "Enabling all stringnormalization options");
                if (mpd_send_all_stringnormalization(partition_state->conn) == false) {
                    mympd_set_mpd_failure(partition_state, "Failure adding command to command list mpd_send_all_stringnormalization");
                }
            }
            else {
                MYMPD_LOG_INFO(partition_state->name, "Disabling all stringnormalization options");
                if (mpd_send_clear_stringnormalization(partition_state->conn) == false) {
                    mympd_set_mpd_failure(partition_state, "Failure adding command to command list mpd_send_clear_stringnormalization");
                }
            }
        }
        if (mympd_client_command_list_end_check(partition_state) == false) {
            sds message = sdsnew("Failure setting protocol options");
            ws_notify(message, partition_state->name);
            FREE_SDS(message);
        }
    }
    return mympd_check_error_and_recover(partition_state, NULL, "protocol options");
}

/**
 * Sets mpd connection settings and features
 * @param partition_state pointer to partition state
 * @return true on success, else false
 */
bool mympd_client_set_connection_options(struct t_partition_state *partition_state) {
    return mympd_client_set_keepalive(partition_state) &&
        mympd_client_set_timeout(partition_state) &&
        mympd_client_set_protocol_options(partition_state) &&
        enable_mpd_tags(partition_state, &partition_state->mpd_state->tags_mympd);
}

/**
 * Disconnects from MPD, sends a notification and execute triggers
 * @param partition_state pointer to partition state
 */
void mympd_client_disconnect(struct t_partition_state *partition_state) {
    mympd_client_disconnect_silent(partition_state);
    send_jsonrpc_event(JSONRPC_EVENT_MPD_DISCONNECTED, partition_state->name);
    mympd_api_request_trigger_event_emit(TRIGGER_MYMPD_DISCONNECTED, partition_state->name, NULL, 0);
}

/**
 * Disconnects from MPD silently
 * @param partition_state pointer to partition state
 */
void mympd_client_disconnect_silent(struct t_partition_state *partition_state) {
    if (partition_state->conn != NULL) {
        MYMPD_LOG_INFO(partition_state->name, "Disconnecting from mpd");
        mpd_connection_free(partition_state->conn);
    }
    partition_state->conn = NULL;
    partition_state->conn_state = MPD_DISCONNECTED;
}

/**
 * Disconnects all MPD partitions
 * @param mympd_state pointer to central myMPD state
 */
void mympd_client_disconnect_all(struct t_mympd_state *mympd_state) {
    struct t_partition_state *partition_state = mympd_state->partition_state;
    while (partition_state != NULL) {
        mympd_client_disconnect(partition_state);
        partition_state = partition_state->next;
    }
}
