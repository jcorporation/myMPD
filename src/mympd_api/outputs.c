/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2024 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include "compile_time.h"
#include "src/mympd_api/outputs.h"

#include "src/lib/jsonrpc.h"
#include "src/mpd_client/errorhandler.h"
#include "src/mpd_client/shortcuts.h"

#include <string.h>

/**
 * Toggles the enabled state of a mpd output
 * @param partition_state pointer to partition state
 * @param output_id mpd output id
 * @param state the state, 1 = enabled, 0 = disabled
 * @param error already allocated sds string to append the error message
 * @return true on success, else false
 */
bool mympd_api_output_toggle(struct t_partition_state *partition_state, unsigned output_id, unsigned state, sds *error) {
    if (state == 1) {
        mpd_run_enable_output(partition_state->conn, output_id);
        return mympd_check_error_and_recover(partition_state, error, "mpd_run_enable_output");
    }
    mpd_run_disable_output(partition_state->conn, output_id);
    return mympd_check_error_and_recover(partition_state, error, "mpd_run_enable_output");
}

/**
 * Gets details of an output
 * @param partition_state pointer to partition state
 * @param buffer already allocated sds string to append the response
 * @param request_id jsonrpc id
 * @param output_name mpd output name
 * @return pointer to buffer
 */
sds mympd_api_output_get(struct t_partition_state *partition_state, sds buffer, unsigned request_id, sds output_name) {
    enum mympd_cmd_ids cmd_id = MYMPD_API_PLAYER_OUTPUT_GET;
    if (mpd_send_outputs(partition_state->conn)) {
        buffer = jsonrpc_respond_start(buffer, cmd_id, request_id);
        int output_count = 0;
        struct mpd_output *output;
        while ((output = mpd_recv_output(partition_state->conn)) != NULL) {
            const char *mpd_output_name = mpd_output_get_name(output);
            if (strcmp(mpd_output_name, output_name) == 0) {
                buffer = tojson_uint(buffer, "id", mpd_output_get_id(output), true);
                buffer = tojson_char(buffer, "name", mpd_output_name, true);
                buffer = tojson_bool(buffer, "state", mpd_output_get_enabled(output), true);
                buffer = tojson_char(buffer, "plugin", mpd_output_get_plugin(output), true);
                buffer = sdscat(buffer, "\"attributes\":{");
                const struct mpd_pair *attributes = mpd_output_first_attribute(output);
                if (attributes != NULL) {
                    buffer = tojson_char(buffer, attributes->name, attributes->value, false);
                    while ((attributes = mpd_output_next_attribute(output)) != NULL) {
                        buffer = sdscatlen(buffer, ",", 1);
                        buffer = tojson_char(buffer, attributes->name, attributes->value, false);
                    }
                }
                buffer = sdscatlen(buffer, "}", 1);
                output_count++;
                mpd_output_free(output);
                break;
            }
            mpd_output_free(output);
        }
        buffer = jsonrpc_end(buffer);
        if (output_count == 0) {
            buffer = jsonrpc_respond_message(buffer, cmd_id, request_id, JSONRPC_FACILITY_MPD, JSONRPC_SEVERITY_ERROR, "Output not found");
        }
    }
    mpd_response_finish(partition_state->conn);
    mympd_check_error_and_recover_respond(partition_state, &buffer, cmd_id, request_id, "mpd_send_outputs");
    return buffer;
}

/**
 * Lists all outputs
 * @param partition_state pointer to partition state
 * @param buffer already allocated sds string to append the response
 * @param request_id jsonrpc id
 * @return pointer to buffer
 */
sds mympd_api_output_list(struct t_partition_state *partition_state, sds buffer, unsigned request_id) {
    enum mympd_cmd_ids cmd_id = MYMPD_API_PLAYER_OUTPUT_LIST;
    if (mpd_send_outputs(partition_state->conn)) {
        buffer = jsonrpc_respond_start(buffer, cmd_id, request_id);
        buffer = sdscat(buffer, "\"data\":[");
        unsigned entity_count = 0;
        struct mpd_output *output;
        while ((output = mpd_recv_output(partition_state->conn)) != NULL) {
            if (entity_count++) {
                buffer = sdscatlen(buffer, ",", 1);
            }
            buffer = sdscatlen(buffer, "{", 1);
            buffer = tojson_uint(buffer, "id", mpd_output_get_id(output), true);
            buffer = tojson_char(buffer, "name", mpd_output_get_name(output), true);
            buffer = tojson_bool(buffer, "state", mpd_output_get_enabled(output), true);
            buffer = tojson_char(buffer, "plugin", mpd_output_get_plugin(output), false);
            buffer = sdscatlen(buffer, "}", 1);
            mpd_output_free(output);
        }
        buffer = sdscatlen(buffer, "],", 2);
        buffer = tojson_uint(buffer, "returnedEntities", entity_count, true);
        buffer = tojson_uint(buffer, "totalEntities", entity_count, false);
        buffer = jsonrpc_end(buffer);
    }
    mpd_response_finish(partition_state->conn);
    mympd_check_error_and_recover_respond(partition_state, &buffer, cmd_id, request_id, "mpd_send_outputs");
    return buffer;
}

bool mympd_api_output_attributes_set(struct t_partition_state *partition_state,
        unsigned output_id, struct t_list *attributes, sds *error)
{
    if (mpd_command_list_begin(partition_state->conn, false)) {
        struct t_list_node *current;
        while ((current = list_shift_first(attributes)) != NULL) {
            bool rc = mpd_send_output_set(partition_state->conn, output_id, current->key, current->value_p);
            list_node_free(current);
            if (rc == false) {
                mympd_set_mpd_failure(partition_state, "Error adding command to command list mpd_send_output_set");
                break;
            }
        }
        mpd_client_command_list_end_check(partition_state);
    }
    mpd_response_finish(partition_state->conn);
    return mympd_check_error_and_recover(partition_state, error, "mpd_send_output_set");
}
