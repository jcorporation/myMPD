/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2024 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

/*! \file
 * \brief myMPD tagart API
 */

#include "compile_time.h"
#include "src/mympd_api/tagart.h"

#include "src/lib/api.h"
#include "src/lib/jsonrpc.h"
#include "src/lib/log.h"
#include "src/mympd_api/trigger.h"

#include <string.h>

/**
 * Fetches the tagart.
 * MPD has no function for this, we rely only on scripts.
 * Local tagart was already read by the webserver thread.
 * @param mympd_state pointer to mympd state
 * @param partition_state pointer to partition specific states
 * @param buffer already allocated sds string for the jsonrpc response
 * @param request_id request id
 * @param conn_id mongoose connection id
 * @param tag mpd tag
 * @param value mpd tag value
 * @return jsonrpc response
 */
sds mympd_api_tagart(struct t_mympd_state *mympd_state, struct t_partition_state *partition_state,
        sds buffer, unsigned request_id, unsigned long conn_id, sds tag, sds value)
{
    #ifdef MYMPD_ENABLE_LUA
        struct t_list arguments;
        list_init(&arguments);
        list_push(&arguments, "tag", 0, tag, NULL);
        list_push(&arguments, "value", 0, value, NULL);
        int n = mympd_api_trigger_execute_http(&mympd_state->trigger_list, TRIGGER_MYMPD_TAGART,
                partition_state->name, conn_id, request_id, &arguments);
        list_clear(&arguments);
        if (n > 0) {
            // return empty buffer, response must be send by triggered script
            if (n > 1) {
                MYMPD_LOG_WARN(partition_state->name, "More than one script triggered for tagart.");
            }
            return buffer;
        }
    #else
        (void)mympd_state;
        (void)conn_id;
    #endif
    MYMPD_LOG_INFO(partition_state->name, "No tagart found for tag \"%s\" = \"%s\"", tag, value);
    buffer = jsonrpc_respond_message(buffer, INTERNAL_API_TAGART, request_id,
            JSONRPC_FACILITY_MPD, JSONRPC_SEVERITY_WARN, "No tagart found.");

    return buffer;
}
