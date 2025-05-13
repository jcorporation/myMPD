/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2025 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

/*! \file
 * \brief myMPD folderart API
 */

#include "compile_time.h"
#include "src/mympd_api/folderart.h"

#include "src/lib/api.h"
#include "src/lib/json/json_rpc.h"
#include "src/lib/log.h"
#include "src/mympd_api/trigger.h"

#include <string.h>

/**
 * Fetches the folderart.
 * MPD has no function for this, we rely only on scripts.
 * Local playlistart was already read by the webserver thread.
 * @param mympd_state pointer to mympd state
 * @param partition_state pointer to partition specific states
 * @param buffer already allocated sds string for the jsonrpc response
 * @param request_id request id
 * @param conn_id mongoose connection id
 * @param path Folderpath
 * @return jsonrpc response
 */
sds mympd_api_folderart(struct t_mympd_state *mympd_state, struct t_partition_state *partition_state,
        sds buffer, unsigned request_id, unsigned long conn_id, sds path)
{
    #ifdef MYMPD_ENABLE_LUA
        struct t_list arguments;
        list_init(&arguments);
        list_push(&arguments, "path", 0, path, NULL);
        int n = mympd_api_trigger_execute_http(&mympd_state->trigger_list, TRIGGER_MYMPD_FOLDERART,
                partition_state->name, conn_id, request_id, &arguments);
        list_clear(&arguments);
        if (n > 0) {
            // return empty buffer, response must be send by triggered script
            if (n > 1) {
                MYMPD_LOG_WARN(partition_state->name, "More than one script triggered for folderart.");
            }
            return buffer;
        }
    #else
        (void)mympd_state;
        (void)conn_id;
    #endif
    MYMPD_LOG_INFO(partition_state->name, "No folderart found for \"%s\"", path);
    buffer = jsonrpc_respond_message(buffer, INTERNAL_API_FOLDERART, request_id,
        JSONRPC_FACILITY_MPD, JSONRPC_SEVERITY_WARN, "No folderart found.");
    return buffer;
}
