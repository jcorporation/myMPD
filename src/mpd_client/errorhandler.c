/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2022 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include "compile_time.h"
#include "errorhandler.h"

#include "../lib/jsonrpc.h"
#include "../lib/log.h"
#include "../lib/mem.h"
#include "../lib/sds_extras.h"
#include "tags.h"

/**
 * Private definitions
 */

/**
 * Response types for error messages
 */
enum response_types {
    RESPONSE_TYPE_JSONRPC_RESPONSE,
    RESPONSE_TYPE_JSONRPC_NOTIFY,
    RESPONSE_TYPE_PLAIN,
    RESPONSE_TYPE_NONE
};

static bool _check_rc_error_and_recover(struct t_partition_state *partition_state, sds *buffer,
        enum mympd_cmd_ids cmd_id, long request_id, enum response_types response_type, bool rc,
        const char *command);
static bool _check_error_and_recover(struct t_partition_state *partition_state, sds *buffer, enum mympd_cmd_ids cmd_id,
        long request_id, enum response_types response_type);

/**
 * Public functions
 */

/**
 * Checks for mpd protocol error and tries to recover it
 * @param partition_state pointer to partition specific states
 * @return true on success else false
 */
bool mympd_check_error_and_recover(struct t_partition_state *partition_state) {
    return _check_error_and_recover(partition_state, NULL, GENERAL_API_UNKNOWN, 0, RESPONSE_TYPE_NONE);
}

/**
 * Checks for mpd protocol error and return code of last mpd command and tries to recover it
 * @param partition_state pointer to partition specific states
 * @param rc return code of last mpd command to check
 * @param command last mpd command
 * @return true on success else false
 */
bool mympd_check_rc_error_and_recover(struct t_partition_state *partition_state, bool rc, const char *command) {
    return _check_rc_error_and_recover(partition_state, NULL, GENERAL_API_UNKNOWN, 0, RESPONSE_TYPE_NONE, rc, command);
}

/**
 * Checks for mpd protocol error and tries to recover it.
 * Creates a jsonrpc response on error.
 * @param partition_state pointer to partition specific states
 * @param buffer pointer to an already allocated sds string for the jsonrpc response
 * @param cmd_id enum mympd_cmd_ids
 * @param request_id jsonrpc request id to respond
 * @return true on success else false
 */
bool mympd_check_error_and_recover_respond(struct t_partition_state *partition_state, sds *buffer,
        enum mympd_cmd_ids cmd_id, long request_id)
{
    return _check_error_and_recover(partition_state, buffer, cmd_id, request_id, RESPONSE_TYPE_JSONRPC_RESPONSE);
}

/**
 * Checks for mpd protocol error and return code of last mpd command and tries to recover it
 * Creates a jsonrpc response on error.
 * @param partition_state pointer to partition specific states
 * @param buffer pointer to an already allocated sds string for the jsonrpc response
 * @param cmd_id enum mympd_cmd_ids
 * @param request_id jsonrpc request id to respond
 * @param rc return code of last mpd command to check
 * @param command last mpd command
 * @return true on success else false
 */
bool mympd_check_rc_error_and_recover_respond(struct t_partition_state *partition_state, sds *buffer,
        enum mympd_cmd_ids cmd_id, long request_id, bool rc, const char *command)
{
    return _check_rc_error_and_recover(partition_state, buffer, cmd_id, request_id, RESPONSE_TYPE_JSONRPC_RESPONSE, rc, command);
}

/**
 * Checks for mpd protocol error and tries to recover it.
 * Creates a jsonrpc notification on error.
 * @param partition_state pointer to partition specific states
 * @param buffer already allocated sds string for the jsonrpc response
 * @return true on success else false
 */
bool mympd_check_error_and_recover_notify(struct t_partition_state *partition_state, sds *buffer) {
    return _check_error_and_recover(partition_state, buffer, GENERAL_API_UNKNOWN, 0, RESPONSE_TYPE_JSONRPC_NOTIFY);
}

/**
 * Checks for mpd protocol error and return code of last mpd command and tries to recover it
 * Creates a jsonrpc notification on error.
 * @param partition_state pointer to partition specific states
 * @param buffer already allocated sds string for the jsonrpc response
 * @param rc return code of last mpd command to check
 * @param command last mpd command
 * @return true on success else false
 */
bool mympd_check_rc_error_and_recover_notify(struct t_partition_state *partition_state, sds *buffer, bool rc,
        const char *command)
{
    return _check_rc_error_and_recover(partition_state, buffer, GENERAL_API_UNKNOWN, 0, RESPONSE_TYPE_JSONRPC_NOTIFY, rc, command);
}

/**
 * Checks for mpd protocol error and tries to recover it.
 * Returns the plain mpd error message.
 * @param partition_state pointer to partition specific states
 * @param buffer already allocated sds string for the mpd error message
 * @return true on success else false
 */
bool mympd_check_error_and_recover_plain(struct t_partition_state *partition_state, sds *buffer) {
    return _check_error_and_recover(partition_state, buffer, GENERAL_API_UNKNOWN, 0, RESPONSE_TYPE_PLAIN);
}

/**
 * Checks for mpd protocol error and return code of last mpd command and tries to recover it
 * Returns the plain mpd error message.
 * @param partition_state pointer to partition specific states
 * @param buffer already allocated sds string for the jsonrpc response
 * @param rc return code of last mpd command to check
 * @param command last mpd command
 * @return true on success else false
 */
bool mympd_check_rc_error_and_recover_plain(struct t_partition_state *partition_state, sds *buffer, bool rc,
        const char *command)
{
    return _check_rc_error_and_recover(partition_state, buffer, GENERAL_API_UNKNOWN, 0, RESPONSE_TYPE_PLAIN, rc, command);
}

/**
 * Checks for mpd protocol error and return code of last mpd command and tries to recover it.
 * Creates always a jsonrpc response.
 * Shortcut for mpd_check_rc_error_and_recover_respond and jsonrpc_respond_ok
 * @param partition_state pointer to partition specific states
 * @param buffer already allocated sds string for the jsonrpc response
 * @param cmd_id enum mympd_cmd_ids
 * @param request_id jsonrpc request id to respond
 * @param rc return code of last mpd command to check
 * @param command last mpd command
 * @param result pointer to bool for result code
 * @return pointer to buffer
 */
sds mympd_respond_with_error_or_ok(struct t_partition_state *partition_state, sds buffer, enum mympd_cmd_ids cmd_id,
        long request_id, bool rc, const char *command, bool *result)
{
    sdsclear(buffer);
    *result = _check_rc_error_and_recover(partition_state, &buffer, cmd_id, request_id, false, rc, command);
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
 * It additionally checks if rc is true
 * @param partition_state pointer to partition specific states
 * @param buffer already allocated sds string for the jsonrpc response
 * @param cmd_id enum mympd_cmd_ids
 * @param request_id jsonrpc request id to respond
 * @param response_type response message type
 * @param rc return code to check additionally
 * @param command command to check for the return code
 * @return true on success else false
 */
static bool _check_rc_error_and_recover(struct t_partition_state *partition_state, sds *buffer,
        enum mympd_cmd_ids cmd_id, long request_id, enum response_types response_type, bool rc,
        const char *command)
{
    //first do normal mpd error checking
    if (_check_error_and_recover(partition_state, buffer, cmd_id, request_id, response_type) == false) {
        MYMPD_LOG_ERROR("Error in response to command %s", command);
        return false;
    }
    //there is no mpd error at connection level but the command failed, return command error
    if (rc == false) {
        if (buffer != NULL &&
            *buffer != NULL)
        {
            switch(response_type) {
                case RESPONSE_TYPE_JSONRPC_RESPONSE:
                    *buffer = jsonrpc_respond_message_phrase(*buffer, cmd_id, request_id, JSONRPC_FACILITY_MPD,
                        JSONRPC_SEVERITY_ERROR, "Error in response to command: %{command}", 2, "command", command);
                    break;
                case RESPONSE_TYPE_JSONRPC_NOTIFY:
                    *buffer = jsonrpc_notify_phrase(*buffer, JSONRPC_FACILITY_MPD, JSONRPC_SEVERITY_ERROR,
                        "Error in response to command: %{command}", 2, "command", command);
                    break;
                default:
                    *buffer = sdscatfmt(*buffer, "Error in response to command: %s", command);
            }
        }
        MYMPD_LOG_ERROR("Error in response to command %s", command);
        return false;
    }
    return true;
}

/**
 * Checks for an mpd error and tries to recover.
 * @param partition_state pointer to partition specific states
 * @param buffer already allocated sds string for the jsonrpc response
 * @param cmd_id enum mympd_cmd_ids
 * @param request_id jsonrpc request id to respond
 * @param response_type response message type
 * @return true on success else false
 */
static bool _check_error_and_recover(struct t_partition_state *partition_state, sds *buffer, enum mympd_cmd_ids cmd_id,
        long request_id, enum response_types response_type)
{
    enum mpd_error error = mpd_connection_get_error(partition_state->conn);
    if (error != MPD_ERROR_SUCCESS) {
        const char *error_msg = mpd_connection_get_error_message(partition_state->conn);
        MYMPD_LOG_ERROR("MPD error: %s (%d)", error_msg , error);
        if (buffer != NULL &&
            *buffer != NULL)
        {
            switch(response_type) {
                case RESPONSE_TYPE_JSONRPC_RESPONSE:
                    *buffer = jsonrpc_respond_message(*buffer, cmd_id, request_id,
                        JSONRPC_FACILITY_MPD, JSONRPC_SEVERITY_ERROR, error_msg);
                    break;
                case RESPONSE_TYPE_JSONRPC_NOTIFY:
                    *buffer = jsonrpc_notify(*buffer, JSONRPC_FACILITY_MPD, JSONRPC_SEVERITY_ERROR, error_msg);
                    break;
                default:
                    *buffer = sdscat(*buffer, error_msg);
            }
        }
        //try to recover from error
        bool recovered = mpd_connection_clear_error(partition_state->conn);
        if (recovered == false) {
            partition_state->conn_state = MPD_FAILURE;
        }
        else {
            mpd_response_finish(partition_state->conn);
            //enable default mpd tags after cleaning error
            enable_mpd_tags(partition_state, &partition_state->mpd_state->tags_mympd);
        }
        return false;
    }
    return true;
}
