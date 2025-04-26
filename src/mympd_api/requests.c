/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2025 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

/*! \file
 * \brief Request creation helpers
 */

#include "compile_time.h"
#include "src/mympd_api/requests.h"

#include "src/lib/api.h"
#include "src/lib/json/json_print.h"
#include "src/lib/json/json_rpc.h"

/**
 * Pushes a MYMPD_API_CACHES_CREATE event to the queue
 * @return true on success, else false
 */
bool mympd_api_request_caches_create(void) {
    struct t_work_request *request = create_request(REQUEST_TYPE_DISCARD, 0, 0, MYMPD_API_CACHES_CREATE, NULL, MPD_PARTITION_DEFAULT);
    request->data = sdscat(request->data, "\"force\":false}}"); //only update if database has changed
    return push_request(request, 0);
}

/**
 * Pushes a MYMPD_API_JUKEBOX_RESTART event to the queue
 * @param partition partition name
 * @return true on success, else false
 */
bool mympd_api_request_jukebox_restart(const char *partition) {
    struct t_work_request *request = create_request(REQUEST_TYPE_DISCARD, 0, 0, MYMPD_API_JUKEBOX_RESTART, "", partition);
    return push_request(request, 0);
}

/**
 * Pushes a INTERNAL_API_TRIGGER_EVENT_EMIT event to the queue
 * @param event trigger event
 * @param partition partition name
 * @param arguments List of arguments
 * @param conn_id Mongoose connection id
 * @return true on success, else false
 */
bool mympd_api_request_trigger_event_emit(enum trigger_events event, const char *partition,
        struct t_list *arguments, unsigned long conn_id)
{
    struct t_work_request *request = create_request(REQUEST_TYPE_DISCARD, conn_id, 0, INTERNAL_API_TRIGGER_EVENT_EMIT, "", partition);
    request->extra = mympd_api_event_data_new(event, arguments);
    request->extra_free = mympd_api_event_data_free_void;
    return push_request(request, 0);
}

/**
 * Pushes a INTERNAL_API_STICKER_FEATURES event to the queue
 * @param feat_sticker stickers enabled?
 * @param feat_advsticker advanced sticker support?
 * @return true on success, else false
 */
bool mympd_api_request_sticker_features(bool feat_sticker, bool feat_advsticker) {
    struct t_work_request *request = create_request(REQUEST_TYPE_DISCARD, 0, 0, INTERNAL_API_STICKER_FEATURES, NULL, "default");
    request->data = tojson_bool(request->data, "sticker", feat_sticker, true);
    request->data = tojson_bool(request->data, "advsticker", feat_advsticker, false);
    request->data = jsonrpc_end(request->data);
    return push_request(request, 0);
}
