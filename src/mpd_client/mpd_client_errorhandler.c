/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2022 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include "mympd_config_defs.h"
#include "mpd_client_errorhandler.h"

#include "../lib/jsonrpc.h"
#include "../lib/log.h"
#include "../lib/mem.h"
#include "../lib/sds_extras.h"
#include "mpd_client_tags.h"

//private definitions

static bool _check_rc_error_and_recover(struct t_mpd_state *mpd_state, sds *buffer,
        enum mympd_cmd_ids cmd_id, long request_id, bool notify, bool rc,
        const char *command);
static bool _check_error_and_recover(struct t_mpd_state *mpd_state, sds *buffer, enum mympd_cmd_ids cmd_id,
        long request_id, bool notify);

//public functions

/**
 * Checks for mpd protocol error and tries to recover it
 * @param mpd_state pointer to t_mpd_state struct
 * @return true on success else false
 */
bool mympd_check_error_and_recover(struct t_mpd_state *mpd_state) {
    return _check_error_and_recover(mpd_state, NULL, GENERAL_API_UNKNOWN, 0, false);
}

/**
 * Checks for mpd protocol error and return code of last mpd command and tries to recover it
 * @param mpd_state pointer to t_mpd_state struct
 * @param rc return code of last mpd command to check
 * @param command last mpd command
 * @return true on success else false
 */
bool mympd_check_rc_error_and_recover(struct t_mpd_state *mpd_state, bool rc, const char *command) {
    return _check_rc_error_and_recover(mpd_state, NULL, GENERAL_API_UNKNOWN, 0, false, rc, command);
}

/**
 * Checks for mpd protocol error and tries to recover it.
 * Creates a jsonrpc response on error.
 * @param mpd_state pointer to t_mpd_state struct
 * @param buffer already allocated sds string for the jsonrpc response
 * @param cmd_id enum mympd_cmd_ids
 * @param request_id jsonrpc request id to respond
 * @param result pointer to bool for result
 * @return pointer to buffer
 */
sds mympd_check_error_and_recover_respond(struct t_mpd_state *mpd_state, sds buffer,
        enum mympd_cmd_ids cmd_id, long request_id, bool *result)
{
    *result = _check_error_and_recover(mpd_state, &buffer, cmd_id, request_id, false);
    return buffer;
}

/**
 * Checks for mpd protocol error and return code of last mpd command and tries to recover it
 * Creates a jsonrpc response on error.
 * @param mpd_state pointer to t_mpd_state struct
 * @param buffer already allocated sds string for the jsonrpc response
 * @param cmd_id enum mympd_cmd_ids
 * @param request_id jsonrpc request id to respond
 * @param rc return code of last mpd command to check
 * @param command last mpd command
 * @param result pointer to bool for result
 * @return pointer to buffer
 */
sds mympd_check_rc_error_and_recover_respond(struct t_mpd_state *mpd_state, sds buffer,
        enum mympd_cmd_ids cmd_id, long request_id, bool rc, const char *command, bool *result)
{
    *result = _check_rc_error_and_recover(mpd_state, &buffer, cmd_id, request_id, false, rc, command);
    return buffer;
}

/**
 * Checks for mpd protocol error and tries to recover it.
 * Creates a jsonrpc notification on error.
 * @param mpd_state pointer to t_mpd_state struct
 * @param buffer already allocated sds string for the jsonrpc response
 * @param result pointer to bool for result
 * @return pointer to buffer
 */
sds mympd_check_error_and_recover_notify(struct t_mpd_state *mpd_state, sds buffer, bool *result) {
    *result = _check_error_and_recover(mpd_state, &buffer, GENERAL_API_UNKNOWN, 0, true);
    return buffer;
}

/**
 * Checks for mpd protocol error and return code of last mpd command and tries to recover it
 * Creates a jsonrpc notification on error.
 * @param mpd_state pointer to t_mpd_state struct
 * @param buffer already allocated sds string for the jsonrpc response
 * @param rc return code of last mpd command to check
 * @param command last mpd command
 * @param result pointer to bool for result
 * @return pointer to buffer
 */
sds mympd_check_rc_error_and_recover_notify(struct t_mpd_state *mpd_state, sds buffer, bool rc,
        const char *command, bool *result)
{
    *result = _check_rc_error_and_recover(mpd_state, &buffer, GENERAL_API_UNKNOWN, 0, false, rc, command);
    return buffer;
}

/**
 * Checks for mpd protocol error and return code of last mpd command and tries to recover it.
 * Creates always a jsonrpc response.
 * Shortcut for mpd_check_rc_error_and_recover_respond and jsonrpc_respond_ok
 * @param mpd_state pointer to t_mpd_state struct
 * @param buffer already allocated sds string for the jsonrpc response
 * @param cmd_id enum mympd_cmd_ids
 * @param request_id jsonrpc request id to respond
 * @param rc return code of last mpd command to check
 * @param command last mpd command
 * @param result pointer to bool for result
 * @return pointer to buffer
 */
sds mympd_respond_with_error_or_ok(struct t_mpd_state *mpd_state, sds buffer, enum mympd_cmd_ids cmd_id,
        long request_id, bool rc, const char *command, bool *result)
{
    sdsclear(buffer);
    *result = _check_rc_error_and_recover(mpd_state, &buffer, cmd_id, request_id, false, rc, command);
    if (*result == false) {
        return buffer;
    }
    return jsonrpc_respond_ok(buffer, cmd_id, request_id, JSONRPC_FACILITY_MPD);
}

/**
 * Creates a jsonrpc respond message for a mpd error
 * @param buffer already allocated sds string for the jsonrpc response
 * @param cmd_id enum mympd_cmd_ids
 * @param request_id jsonrpc request id to respond
 * @param command mpd command that failed
 * @return pointer to buffer
 */
sds mympd_respond_with_command_error(sds buffer, enum mympd_cmd_ids cmd_id, long request_id, const char *command) {
    return jsonrpc_respond_message_phrase(buffer, cmd_id, request_id,
        JSONRPC_FACILITY_MPD, JSONRPC_SEVERITY_ERROR, "Error in response to command: %{command}",
        2, "command", command);
}

//private functions

static bool _check_rc_error_and_recover(struct t_mpd_state *mpd_state, sds *buffer,
        enum mympd_cmd_ids cmd_id, long request_id, bool notify, bool rc,
        const char *command)
{
    if (_check_error_and_recover(mpd_state, buffer, cmd_id, request_id, notify) == false) {
        MYMPD_LOG_ERROR("Error in response to command %s", command);
        return false;
    }
    if (rc == false) {
        if (buffer != NULL &&
            *buffer != NULL)
        {
            if (notify == false) {
                *buffer = mympd_respond_with_command_error(*buffer, cmd_id, request_id, command);
            }
            else {
                *buffer = jsonrpc_notify_phrase(*buffer, JSONRPC_FACILITY_MPD, JSONRPC_SEVERITY_ERROR,
                    "Error in response to command: %{command}", 2, "command", command);
            }
        }
        MYMPD_LOG_ERROR("Error in response to command %s", command);
        return false;
    }
    return true;
}

static bool _check_error_and_recover(struct t_mpd_state *mpd_state, sds *buffer, enum mympd_cmd_ids cmd_id,
        long request_id, bool notify)
{
    enum mpd_error error = mpd_connection_get_error(mpd_state->conn);
    if (error != MPD_ERROR_SUCCESS) {
        const char *error_msg = mpd_connection_get_error_message(mpd_state->conn);
        MYMPD_LOG_ERROR("MPD error: %s (%d)", error_msg , error);
        if (buffer != NULL &&
            *buffer != NULL)
        {
            if (notify == false) {
                *buffer = jsonrpc_respond_message(*buffer, cmd_id, request_id,
                    JSONRPC_FACILITY_MPD, JSONRPC_SEVERITY_ERROR, error_msg);
            }
            else {
                *buffer = jsonrpc_notify(*buffer, JSONRPC_FACILITY_MPD, JSONRPC_SEVERITY_ERROR, error_msg);
            }
        }
        //try to recover from error
        bool recovered = mpd_connection_clear_error(mpd_state->conn);
        if (recovered == false) {
            mpd_state->conn_state = MPD_FAILURE;
        }
        else {
            mpd_response_finish(mpd_state->conn);
            //enable default mpd tags after cleaning error
            enable_mpd_tags(mpd_state, &mpd_state->tag_types_mympd);
        }
        return false;
    }
    return true;
}
