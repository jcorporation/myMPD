/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2025 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

/*! \file
 * \brief myMPD playlistart API
 */

#include "compile_time.h"
#include "src/mympd_api/playlistart.h"

#include "src/lib/api.h"
#include "src/lib/json/json_rpc.h"
#include "src/lib/log.h"
#include "src/mympd_api/trigger.h"

#include <string.h>

/**
 * Fetches the playlistart.
 * MPD has no function for this, we rely only on scripts.
 * Local playlistart was already read by the webserver thread.
 * @param mympd_state pointer to mympd state
 * @param partition_state pointer to partition specific states
 * @param buffer already allocated sds string for the jsonrpc response
 * @param request_id request id
 * @param conn_id mongoose connection id
 * @param name Playlist name
 * @param type Playlist type
 * @return jsonrpc response
 */
sds mympd_api_playlistart(struct t_mympd_state *mympd_state, struct t_partition_state *partition_state,
        sds buffer, unsigned request_id, unsigned long conn_id, sds name, sds type)
{
    #ifdef MYMPD_ENABLE_LUA
        struct t_list arguments;
        list_init(&arguments);
        list_push(&arguments, "name", 0, name, NULL);
        list_push(&arguments, "type", 0, type, NULL);
        int n = mympd_api_trigger_execute_http(&mympd_state->trigger_list, TRIGGER_MYMPD_PLAYLISTART,
                partition_state->name, conn_id, request_id, &arguments);
        list_clear(&arguments);
        if (n > 0) {
            // return empty buffer, response must be send by triggered script
            if (n > 1) {
                MYMPD_LOG_WARN(partition_state->name, "More than one script triggered for playlistart.");
            }
            return buffer;
        }
    #else
        (void)request_id;
        (void)mympd_state;
        (void)conn_id;
    #endif
    MYMPD_LOG_INFO(partition_state->name, "No playlistart found for %s \"%s\"", type, name);
    buffer = sdscat(buffer, type);
    return buffer;
}
