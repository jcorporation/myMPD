/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2026 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

/*! \file
 * \brief myMPD lyrics API
 */

#include "compile_time.h"
#include "src/mympd_api/lyrics.h"

#include "src/lib/json/json_print.h"
#include "src/lib/json/json_rpc.h"
#include "src/lib/log.h"
#include "src/mympd_api/trigger.h"

#include <string.h>

/**
 * Triggers scripts to get lyrics.
 * MPD can not read lyrics.
 * @param mympd_state pointer to mympd_state
 * @param buffer buffer to write the response
 * @param uri song uri 
 * @param partition mpd partition
 * @param conn_id mongoose connection id
 * @param request_id jsonrpc id
 * @return pointer to buffer
 */
sds mympd_api_lyrics_get(struct t_mympd_state *mympd_state, sds buffer,
        sds uri, sds partition, unsigned long conn_id, unsigned request_id)
{
    #ifdef MYMPD_ENABLE_LUA
        struct t_list arguments;
        list_init(&arguments);
        list_push(&arguments, "uri", 0, uri, NULL);
        int n = mympd_api_trigger_execute_http(&mympd_state->trigger_list, TRIGGER_MYMPD_LYRICS,
                partition, conn_id, request_id, &arguments);
        list_clear(&arguments);
        if (n > 0) {
            // return empty buffer, response must be send by triggered script
            if (n > 1) {
                MYMPD_LOG_WARN(partition, "More than one script triggered for lyrics.");
            }
            return buffer;
        }
    #else
        (void)mympd_state;
        (void)uri;
        (void)partition;
        (void)conn_id;
    #endif
    // no trigger
    buffer = jsonrpc_respond_message(buffer, MYMPD_API_LYRICS_GET, request_id,
        JSONRPC_FACILITY_LYRICS, JSONRPC_SEVERITY_INFO, "No lyrics found");
    return buffer;
}
