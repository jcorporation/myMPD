/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2024 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include "compile_time.h"
#include "src/mpd_client/errorhandler.h"

#include "dist/libmympdclient/include/mpd/client.h"
#include "src/lib/jsonrpc.h"
#include "src/lib/log.h"
#include "src/lib/timer.h"
#include "src/mpd_client/connection.h"
#include "src/mpd_client/tags.h"

#include <errno.h>
#include <string.h>

/**
 * Private definitions
 */

static bool check_error_and_recover(struct t_partition_state *partition_state, sds *buffer, enum mympd_cmd_ids cmd_id,
        unsigned request_id, enum jsonrpc_response_types response_type, const char *command);

/**
 * Public functions
 */

/**
 * Sets the MPD_FAILURE state for the partition.
 * myMPD disconnects and tries a reconnect.
 */
void mympd_set_mpd_failure(struct t_partition_state *partition_state, const char *errormessage) {
    MYMPD_LOG_ERROR(partition_state->name, "%s", errormessage);
    mpd_client_disconnect(partition_state);
    mympd_timer_set(partition_state->timer_fd_mpd_connect, 0, 5);
}

/**
 * Checks for mpd protocol error and tries to recover it
 * @param partition_state pointer to partition specific states
 * @param error pointer to an already allocated sds string for the error message
 * @param command last mpd command
 * @return true on success, else false
 */
bool mympd_check_error_and_recover(struct t_partition_state *partition_state, sds *error, const char *command) {
    return check_error_and_recover(partition_state, error, GENERAL_API_UNKNOWN, 0, RESPONSE_TYPE_NONE, command);
}

/**
 * Checks for mpd protocol error and tries to recover it.
 * Creates a jsonrpc response on error.
 * @param partition_state pointer to partition specific states
 * @param buffer pointer to an already allocated sds string for the jsonrpc response
 * @param cmd_id enum mympd_cmd_ids
 * @param request_id jsonrpc request id to respond
 * @param command last mpd command
 * @return true on success, else false
 */
bool mympd_check_error_and_recover_respond(struct t_partition_state *partition_state, sds *buffer,
        enum mympd_cmd_ids cmd_id, unsigned request_id, const char *command)
{
    return check_error_and_recover(partition_state, buffer, cmd_id, request_id, RESPONSE_TYPE_JSONRPC_RESPONSE, command);
}

/**
 * Checks for mpd protocol error and tries to recover it.
 * Creates a jsonrpc notification on error.
 * @param partition_state pointer to partition specific states
 * @param buffer already allocated sds string for the jsonrpc response
 * @param command last mpd command
 * @return true on success, else false
 */
bool mympd_check_error_and_recover_notify(struct t_partition_state *partition_state, sds *buffer, const char *command) {
    return check_error_and_recover(partition_state, buffer, GENERAL_API_UNKNOWN, 0, RESPONSE_TYPE_JSONRPC_NOTIFY, command);
}

/**
 * Checks for mpd protocol error and tries to recover it.
 * Returns the plain mpd error message.
 * @param partition_state pointer to partition specific states
 * @param buffer already allocated sds string for the mpd error message
 * @param command last mpd command
 * @return true on success, else false
 */
bool mympd_check_error_and_recover_plain(struct t_partition_state *partition_state, sds *buffer, const char *command) {
    return check_error_and_recover(partition_state, buffer, GENERAL_API_UNKNOWN, 0, RESPONSE_TYPE_PLAIN, command);
}

/**
 * Checks for mpd protocol error and return code of last mpd command and tries to recover it.
 * Creates always a jsonrpc response.
 * Shortcut for mympd_check_rc_error_and_recover and jsonrpc_respond_ok
 * @param partition_state pointer to partition specific states
 * @param buffer already allocated sds string for the jsonrpc response
 * @param cmd_id enum mympd_cmd_ids
 * @param request_id jsonrpc request id to respond
 * @param command last mpd command
 * @param result pointer to bool for result code
 * @return pointer to buffer
 */
sds mympd_respond_with_error_or_ok(struct t_partition_state *partition_state, sds buffer, enum mympd_cmd_ids cmd_id,
        unsigned request_id, const char *command, bool *result)
{
    *result = check_error_and_recover(partition_state, &buffer, cmd_id, request_id, RESPONSE_TYPE_JSONRPC_RESPONSE, command);
    if (*result == false) {
        return buffer;
    }
    return jsonrpc_respond_ok(buffer, cmd_id, request_id, JSONRPC_FACILITY_MPD);
}

/**
 * Private functions
 */

/**
 * Checks for an mpd error and tries to recover.
 * @param partition_state pointer to partition specific states
 * @param buffer already allocated sds string for the jsonrpc response
 * @param cmd_id enum mympd_cmd_ids
 * @param request_id jsonrpc request id to respond
 * @param response_type response message type
 * @param command command to check for the error
 * @return true on success, else false
 */
static bool check_error_and_recover(struct t_partition_state *partition_state, sds *buffer, enum mympd_cmd_ids cmd_id,
        unsigned request_id, enum jsonrpc_response_types response_type, const char *command)
{
    if (partition_state->conn == NULL) {
        mympd_set_mpd_failure(partition_state, "Unrecoverable MPD error");
        return false;
    }
    enum mpd_error error = mpd_connection_get_error(partition_state->conn);
    if (error == MPD_ERROR_SUCCESS) {
        return true;
    }

    const char *error_msg = mpd_connection_get_error_message(partition_state->conn);
    if (error == MPD_ERROR_SERVER) {
        enum mpd_server_error server_error = mpd_connection_get_server_error(partition_state->conn);
        MYMPD_LOG_ERROR(partition_state->name, "MPD error for command %s: %s (%d, %d)", command, error_msg , error, server_error);
    }
    else {
        MYMPD_LOG_ERROR(partition_state->name, "MPD error for command %s: %s (%d)", command, error_msg , error);
    }
    if (buffer != NULL &&
        *buffer != NULL)
    {
        sdsclear(*buffer);
        switch(response_type) {
            case RESPONSE_TYPE_JSONRPC_RESPONSE:
                *buffer = jsonrpc_respond_message_phrase(*buffer, cmd_id, request_id,
                    JSONRPC_FACILITY_MPD, JSONRPC_SEVERITY_ERROR, "MPD error for command %{cmd}: %{msg}", 4, "cmd", command, "msg", error_msg);
                break;
            case RESPONSE_TYPE_JSONRPC_NOTIFY:
                *buffer = jsonrpc_notify_phrase(*buffer,
                    JSONRPC_FACILITY_MPD, JSONRPC_SEVERITY_ERROR, "MPD error for command %{cmd}: %{msg}", 4, "cmd", command, "msg", error_msg);
                break;
            default:
                *buffer = sdscat(*buffer, error_msg);
        }
    }
    //try to recover from error
    if (mpd_connection_clear_error(partition_state->conn) == false) {
        mympd_set_mpd_failure(partition_state, "Unrecoverable MPD error");
    }
    else {
        mpd_response_finish(partition_state->conn);
        //enable default mpd tags after recovering from error
        enable_mpd_tags(partition_state, &partition_state->mpd_state->tags_mympd);
    }
    return false;
}
