/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2024 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

/*! \file
 * \brief Request creation helpers
 */

#include "compile_time.h"
#include "src/mympd_api/requests.h"

#include "src/lib/api.h"
#include "src/lib/jsonrpc.h"

/**
 * Pushes a MYMPD_API_CACHES_CREATE to the queue
 * @return true on success, else false
 */
bool mympd_api_request_caches_create(void) {
    struct t_work_request *request = create_request(REQUEST_TYPE_DISCARD, 0, 0, MYMPD_API_CACHES_CREATE, NULL, MPD_PARTITION_DEFAULT);
    request->data = sdscat(request->data, "\"force\":false}}"); //only update if database has changed
    return push_request(request, 0);
}

/**
 * Pushes a MYMPD_API_JUKEBOX_RESTART to the queue
 * @param partition partition name
 * @return true on success, else false
 */
bool mympd_api_request_jukebox_restart(const char *partition) {
    struct t_work_request *request = create_request(REQUEST_TYPE_DISCARD, 0, 0, MYMPD_API_JUKEBOX_RESTART, NULL, partition);
    request->data = sdscatlen(request->data, "}}", 2);
    return push_request(request, 0);
}

/**
 * Pushes a INTERNAL_API_TRIGGER_EVENT_EMIT to the queue
 * @param event trigger event
 * @param partition partition name
 * @return true on success, else false
 */
bool mympd_api_request_trigger_event_emit(enum trigger_events event, const char *partition) {
    struct t_work_request *request = create_request(REQUEST_TYPE_DISCARD, 0, 0, INTERNAL_API_TRIGGER_EVENT_EMIT, NULL, partition);
    request->data = tojson_int(request->data, "event", event, false);
    request->data = sdscatlen(request->data, "}}", 2);
    return push_request(request, 0);
}

/**
 * Pushes a INTERNAL_API_STICKER_FEATURES to the queue
 * @param feat_sticker stickers enabled?
 * @param feat_sticker_sort_window sticker sort window supported?
 * @param feat_sticker_int sticker int value supported?
 * @return true on success, else false
 */
bool mympd_api_request_sticker_features(bool feat_sticker, bool feat_sticker_sort_window, bool feat_sticker_int) {
    struct t_work_request *request = create_request(REQUEST_TYPE_DISCARD, 0, 0, INTERNAL_API_STICKER_FEATURES, NULL, "default");
    request->data = tojson_bool(request->data, "sticker", feat_sticker, true);
    request->data = tojson_bool(request->data, "sticker_sort_window", feat_sticker_sort_window, true);
    request->data = tojson_bool(request->data, "sticker_int", feat_sticker_int, false);
    request->data = sdscatlen(request->data, "}}", 2);
    return push_request(request, 0);
}
