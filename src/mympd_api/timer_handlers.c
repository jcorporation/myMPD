/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2024 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include "compile_time.h"
#include "src/mympd_api/timer_handlers.h"

#include "src/lib/api.h"
#include "src/lib/jsonrpc.h"
#include "src/lib/list.h"
#include "src/lib/log.h"
#include "src/lib/msg_queue.h"
#include "src/lib/mympd_state.h"
#include "src/lib/sds_extras.h"
#include "src/mpd_client/errorhandler.h"
#include "src/mpd_client/jukebox.h"
#include "src/mpd_client/shortcuts.h"
#include "src/mpd_client/volume.h"
#include "src/mympd_api/requests.h"

#include <string.h>

/**
 * Private definitions
 */

static void timer_handler_covercache_crop(void);
static void timer_handler_smartpls_update(void);
static void timer_handler_caches_create(void);

/**
 * Public functions
 */

/**
 * Handles timer by timer_id, this is only used for internal timers
 * @param timer_id the internal timer_id from enum timer_ids
 * @param definition the timer definition - not used
 */
void timer_handler_by_id(unsigned timer_id, struct t_timer_definition *definition) {
    (void) definition; // not used
    switch(timer_id) {
        case TIMER_ID_COVERCACHE_CROP:
            timer_handler_covercache_crop();
            break;
        case TIMER_ID_SMARTPLS_UPDATE:
            timer_handler_smartpls_update();
            break;
        case TIMER_ID_CACHES_CREATE:
            timer_handler_caches_create();
            break;
        default:
            MYMPD_LOG_WARN(NULL, "Unhandled timer_id");
    }
}

/**
 * Handles user defined timers
 * @param timer_id the timer id
 * @param definition the timer definition
 */
void timer_handler_select(unsigned timer_id, struct t_timer_definition *definition) {
    MYMPD_LOG_INFO(definition->partition, "Start timer_handler_select for timer \"%s\" (%u)", definition->name, timer_id);
    if (strcmp(definition->action, "player") == 0 && strcmp(definition->subaction, "stopplay") == 0) {
        struct t_work_request *request = create_request(REQUEST_TYPE_DISCARD, 0, 0, MYMPD_API_PLAYER_STOP, NULL, definition->partition);
        request->data = jsonrpc_end(request->data);
        mympd_queue_push(mympd_api_queue, request, 0);
    }
    else if (strcmp(definition->action, "player") == 0 && strcmp(definition->subaction, "startplay") == 0) {
        struct t_work_request *request = create_request(REQUEST_TYPE_DISCARD, 0, 0, INTERNAL_API_TIMER_STARTPLAY, NULL, definition->partition);
        request->data = tojson_uint(request->data, "volume", definition->volume, true);
        request->data = tojson_sds(request->data, "plist", definition->playlist, true);
        request->data = tojson_sds(request->data, "preset", definition->preset, false);
        request->data = jsonrpc_end(request->data);
        mympd_queue_push(mympd_api_queue, request, 0);
    }
    else if (strcmp(definition->action, "script") == 0) {
        struct t_work_request *request = create_request(REQUEST_TYPE_DISCARD, 0, 0, MYMPD_API_SCRIPT_EXECUTE, NULL, definition->partition);
        request->data = tojson_sds(request->data, "script", definition->subaction, true);
        request->data = sdscat(request->data, "\"arguments\":{");
        struct t_list_node *argument = definition->arguments.head;
        int i = 0;
        while (argument != NULL) {
            if (i++) {
                request->data = sdscatlen(request->data, ",", 1);
            }
            request->data = tojson_sds(request->data, argument->key, argument->value_p, false);
            argument = argument->next;
        }
        request->data = sdscatlen(request->data, "}}}", 3);
        mympd_queue_push(mympd_api_queue, request, 0);
    }
    else {
        MYMPD_LOG_ERROR(definition->partition, "Unknown timer action: %s - %s", definition->action, definition->subaction);
    }
}

/**
 * This function is used by INTERNAL_API_TIMER_STARTPLAY and is called from the mympd_api_handler
 * @param partition_state pointer to the partition_state
 * @param volume mpd volume to set
 * @param playlist the mpd playlist to load
 * @param preset the preset to load
 * @return true on success else false
 */
bool mympd_api_timer_startplay(struct t_partition_state *partition_state,
        unsigned volume, sds playlist, sds preset)
{
    //disable jukebox to prevent adding songs to queue from old jukebox queue list
    enum jukebox_modes old_jukebox_mode = partition_state->jukebox.mode;
    partition_state->jukebox.mode = JUKEBOX_OFF;

    int old_volume = mpd_client_get_volume(partition_state);

    enum jukebox_modes jukebox_mode = JUKEBOX_OFF;
    if (sdslen(preset) > 0) {
        //get the jukebox mode from the preset
        struct t_list_node *preset_value = list_get_node(&partition_state->preset_list, preset);
        if (preset_value != NULL) {
            sds jukebox_mode_str = NULL;
            if (json_get_string_max(preset_value->value_p, "$.jukeboxMode", &jukebox_mode_str, vcb_isname, NULL) == true) {
                jukebox_mode = jukebox_mode_parse(jukebox_mode_str);
            }
            FREE_SDS(jukebox_mode_str);
        }
    }

    if (mpd_command_list_begin(partition_state->conn, false)) {
        if (mpd_send_stop(partition_state->conn) == false) {
            mympd_set_mpd_failure(partition_state, "Error adding command to command list mpd_send_stop");
        }
        if (old_volume != -1) {
            if (mpd_send_set_volume(partition_state->conn, volume) == false) {
                mympd_set_mpd_failure(partition_state, "Error adding command to command list mpd_send_set_volume");
            }
        }

        if (sdslen(playlist) > 0 &&
            jukebox_mode == JUKEBOX_OFF)
        {
            //load selected playlist if in preset jukebox is disabled
            if (mpd_send_clear(partition_state->conn) == false ||
                mpd_send_load(partition_state->conn, playlist) == false ||
                mpd_send_play(partition_state->conn) == false)
            {
                mympd_set_mpd_failure(partition_state, "Error adding command to command list");
            }
        }

        if (jukebox_mode != JUKEBOX_OFF) {
            //clear the queue if jukebox is enabled through preset
            if (mpd_send_clear(partition_state->conn) == false) {
                mympd_set_mpd_failure(partition_state, "Error adding command to command list mpd_send_clear");
            }
        }
        mpd_client_command_list_end_check(partition_state);
    }
    mpd_response_finish(partition_state->conn);
    //restore old jukebox mode
    partition_state->jukebox.mode = old_jukebox_mode;

    if (sdslen(preset) > 0) {
        //load the preset
        struct t_work_request *request = create_request(REQUEST_TYPE_DISCARD, 0, 0, MYMPD_API_PRESET_APPLY, NULL, partition_state->name);
        request->data = tojson_sds(request->data, "name", preset, false);
        request->data = jsonrpc_end(request->data);
        mympd_queue_push(mympd_api_queue, request, 0);
    }

    return mympd_check_error_and_recover(partition_state, NULL, "command_list");
}

/**
 * Private functions
 */

/**
 * Timer handler for timer_id TIMER_ID_COVERCACHE_CROP
 */
static void timer_handler_covercache_crop(void) {
    MYMPD_LOG_INFO(NULL, "Start timer_handler_covercache_crop");
    struct t_work_request *request = create_request(REQUEST_TYPE_DISCARD, 0, 0, MYMPD_API_COVERCACHE_CROP, NULL, MPD_PARTITION_DEFAULT);
    request->data = jsonrpc_end(request->data);
    mympd_queue_push(mympd_api_queue, request, 0);
}

/**
 * Timer handler for timer_id TIMER_ID_SMARTPLS_UPDATE
 */
static void timer_handler_smartpls_update(void) {
    MYMPD_LOG_INFO(NULL, "Start timer_handler_smartpls_update");
    struct t_work_request *request = create_request(REQUEST_TYPE_DISCARD, 0, 0, MYMPD_API_SMARTPLS_UPDATE_ALL, NULL, MPD_PARTITION_DEFAULT);
    request->data = sdscat(request->data, "\"force\":false}}"); //only update if database has changed
    mympd_queue_push(mympd_api_queue, request, 0);
}

/**
 * Timer handler for timer_id TIMER_ID_CACHES_CREATE
 */
static void timer_handler_caches_create(void) {
    MYMPD_LOG_INFO(NULL, "Start timer_handler_caches_create");
    mympd_api_request_caches_create();
}
