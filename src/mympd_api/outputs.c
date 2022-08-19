/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2022 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include "compile_time.h"
#include "outputs.h"

#include "../lib/jsonrpc.h"
#include "../lib/log.h"
#include "../lib/lua_mympd_state.h"
#include "../lib/sds_extras.h"
#include "../lib/utility.h"
#include "../mpd_client/errorhandler.h"
#include "src/lib/api.h"

#include <string.h>

/**
 * Lists output details
 * @param partition_state pointer to partition state
 * @param buffer already allocated sds string to append the response
 * @param request_id jsonrpc id
 * @return pointer to buffer
 */
sds mympd_api_output_list(struct t_partition_state *partition_state, sds buffer, long request_id) {
    enum mympd_cmd_ids cmd_id = MYMPD_API_PLAYER_OUTPUT_LIST;
    bool rc = mpd_send_outputs(partition_state->conn);
    if (mympd_check_rc_error_and_recover_respond(partition_state, &buffer, cmd_id, request_id, rc, "mpd_send_outputs") == false) {
        return buffer;
    }

    buffer = jsonrpc_respond_start(buffer, cmd_id, request_id);
    buffer = sdscat(buffer, "\"data\":[");
    int output_count = 0;
    struct mpd_output *output;
    while ((output = mpd_recv_output(partition_state->conn)) != NULL) {
        if (output_count++) {
            buffer = sdscatlen(buffer, ",", 1);
        }
        buffer = sdscatlen(buffer, "{", 1);
        buffer = tojson_uint(buffer, "id", mpd_output_get_id(output), true);
        buffer = tojson_char(buffer, "name", mpd_output_get_name(output), true);
        buffer = tojson_long(buffer, "state", mpd_output_get_enabled(output), true);
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
        buffer = sdscatlen(buffer, "}}", 2);
        mpd_output_free(output);
    }
    mpd_response_finish(partition_state->conn);
    if (mympd_check_error_and_recover_respond(partition_state, &buffer, cmd_id, request_id) == false) {
        return buffer;
    }

    buffer = sdscatlen(buffer, "],", 2);
    buffer = tojson_char(buffer, "partition", partition_state->name, true);
    buffer = tojson_long(buffer, "numOutputs", output_count, false);
    buffer = jsonrpc_end(buffer);

    return buffer;
}
