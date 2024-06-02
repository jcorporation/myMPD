/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2024 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include "compile_time.h"
#include "src/mympd_api/channel.h"

#include "src/lib/api.h"
#include "src/lib/jsonrpc.h"
#include "src/lib/sds_extras.h"
#include "src/mpd_client/errorhandler.h"
#include <string.h>

/**
 * Returns a list of all MPD channels
 * @param partition_state pointer to partition_state
 * @param buffer sds string to append response
 * @param request_id jsonrpc id
 * @return pointer to buffer
 */
sds mympd_api_channel_list(struct t_partition_state *partition_state, sds buffer, unsigned request_id) {
    enum mympd_cmd_ids cmd_id = MYMPD_API_CHANNEL_LIST;
    if (mpd_send_channels(partition_state->conn)) {
        buffer = jsonrpc_respond_start(buffer, cmd_id, request_id);
        buffer = sdscat(buffer, "\"data\":[");
        unsigned entity_count = 0;
        struct mpd_pair *pair;
        while ((pair = mpd_recv_channel_pair(partition_state->conn)) != NULL) {
            if (entity_count++) {
                buffer = sdscatlen(buffer, ",", 1);
            }
            buffer = sds_catjson(buffer, pair->value, strlen(pair->value));
            mpd_return_pair(partition_state->conn, pair);
        }
        buffer = sdscatlen(buffer, "],", 2);
        buffer = tojson_uint(buffer, "returnedEntities", entity_count, true);
        buffer = tojson_uint(buffer, "totalEntities", entity_count, false);
        buffer = jsonrpc_end(buffer);
    }
    mpd_response_finish(partition_state->conn);
    mympd_check_error_and_recover_respond(partition_state, &buffer, cmd_id, request_id, "mpd_send_channels");
    return buffer;
}

/**
 * Returns a list of all messages in all subscribed channels
 * @param partition_state pointer to partition_state
 * @param buffer sds string to append response
 * @param request_id jsonrpc id
 * @return pointer to buffer
 */
sds mympd_api_channel_messages_read(struct t_partition_state *partition_state, sds buffer, unsigned request_id) {
    enum mympd_cmd_ids cmd_id = MYMPD_API_CHANNEL_MESSAGES_READ;
    if (mpd_send_read_messages(partition_state->conn)) {
        buffer = jsonrpc_respond_start(buffer, cmd_id, request_id);
        buffer = sdscat(buffer, "\"data\":[");
        unsigned entity_count = 0;
        struct mpd_pair *pair;
        while ((pair = mpd_recv_pair(partition_state->conn)) != NULL) {
            if (entity_count > 0) {
                buffer = sdscatlen(buffer, ",", 1);
            }
            if (strcmp(pair->name, "channel") == 0) {
                buffer = sdscatlen(buffer, "{", 1);
                buffer = tojson_char(buffer, "channel", pair->value, true);
            }
            else {
                buffer = tojson_char(buffer, "message", pair->value, false);
                buffer = sdscatlen(buffer, "}", 1);
                entity_count++;
            }
            mpd_return_pair(partition_state->conn, pair);
        }
        buffer = sdscatlen(buffer, "],", 2);
        buffer = tojson_uint(buffer, "returnedEntities", entity_count, true);
        buffer = tojson_uint(buffer, "totalEntities", entity_count, false);
        buffer = jsonrpc_end(buffer);
    }
    mpd_response_finish(partition_state->conn);
    mympd_check_error_and_recover_respond(partition_state, &buffer, cmd_id, request_id, "mpd_send_channels");
    return buffer;
}
