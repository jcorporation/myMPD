/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2022 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include "mympd_config_defs.h"
#include "mympd_api_timer_handlers.h"

#include "../lib/api.h"
#include "../lib/covercache.h"
#include "../lib/jsonrpc.h"
#include "../lib/log.h"
#include "../lib/sds_extras.h"
#include "../mpd_client/mpd_client_errorhandler.h"
#include "../mpd_client/mpd_client_jukebox.h"
#include "../mpd_client/mpd_client_volume.h"

#include <string.h>

//private definitions
static void timer_handler_covercache_crop(void);
static void timer_handler_smartpls_update(void);
static void timer_handler_caches_create(void);

//public functions

/**
 * Handles timer by timer_id, this is only used for internal timers
 * @param timer_id the internal timer_id from enum timer_ids
 * @param definition the timer definition - not used
 */
void timer_handler_by_id(int timer_id, struct t_timer_definition *definition) {
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
            MYMPD_LOG_WARN("Unhandled timer_id");
    }
}

/**
 * Handles user defined timers
 * @param timer_id the timer id
 * @param definition the timer definition
 */
void timer_handler_select(int timer_id, struct t_timer_definition *definition) {
    MYMPD_LOG_INFO("Start timer_handler_select for timer \"%s\" (%d)", definition->name, timer_id);
    if (strcmp(definition->action, "player") == 0 && strcmp(definition->subaction, "stopplay") == 0) {
        struct t_work_request *request = create_request(-1, 0, MYMPD_API_PLAYER_STOP, NULL);
        request->data = sdscatlen(request->data, "}}", 2);
        mympd_queue_push(mympd_api_queue, request, 0);
    }
    else if (strcmp(definition->action, "player") == 0 && strcmp(definition->subaction, "startplay") == 0) {
        struct t_work_request *request = create_request(-1, 0, INTERNAL_API_TIMER_STARTPLAY, NULL);
        request->data = tojson_uint(request->data, "volume", definition->volume, true);
        request->data = tojson_sds(request->data, "plist", definition->playlist, true);
        request->data = tojson_long(request->data, "jukeboxMode", definition->jukebox_mode, false);
        request->data = sdscatlen(request->data, "}}", 2);
        mympd_queue_push(mympd_api_queue, request, 0);
    }
    else if (strcmp(definition->action, "script") == 0) {
        struct t_work_request *request = create_request(-1, 0, MYMPD_API_SCRIPT_EXECUTE, NULL);
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
        MYMPD_LOG_ERROR("Unknown timer action: %s - %s", definition->action, definition->subaction);
    }
}

/**
 * This function is used by INTERNAL_API_TIMER_STARTPLAY and is called from the mympd_api_handler
 * @param partition_state pointer to the partition_state
 * @param volume mpd volume to set
 * @param playlist the mpd playlist to use
 * @param jukebox_mode the jukebox mode to set
 * @return true on success else false
 */
bool mympd_api_timer_startplay(struct t_partition_state *partition_state,
        unsigned volume, sds playlist, enum jukebox_modes jukebox_mode)
{
    //disable jukebox to prevent adding songs to queue from old jukebox queue list
    enum jukebox_modes old_jukebox_mode = partition_state->jukebox_mode;
    partition_state->jukebox_mode = JUKEBOX_OFF;

    int old_volume = mpd_client_get_volume(partition_state);

    bool rc = false;
    if (mpd_command_list_begin(partition_state->conn, false)) {
        rc = mpd_send_stop(partition_state->conn);
        if (rc == false) {
            MYMPD_LOG_ERROR("Error adding command to command list mpd_send_stop");
        }
        if (old_volume != -1) {
            rc = mpd_send_set_volume(partition_state->conn, volume);
            if (rc == false) {
                MYMPD_LOG_ERROR("Error adding command to command list mpd_send_set_volume");
            }
        }
        rc = mpd_send_clear(partition_state->conn);
        if (rc == false) {
            MYMPD_LOG_ERROR("Error adding command to command list mpd_send_clear");
        }
        if (jukebox_mode == JUKEBOX_OFF) {
            rc = mpd_send_load(partition_state->conn, playlist);
            if (rc == false) {
                MYMPD_LOG_ERROR("Error adding command to command list mpd_send_load");
            }
        }
        else {
            rc = mpd_send_consume(partition_state->conn, true);
            if (rc == false) {
                MYMPD_LOG_ERROR("Error adding command to command list mpd_send_consume");
            }
        }
        rc = mpd_send_single_state(partition_state->conn, MPD_SINGLE_OFF);
        if (rc == false) {
            MYMPD_LOG_ERROR("Error adding command to command list mpd_send_single");
        }
        if (jukebox_mode == JUKEBOX_OFF) {
            rc = mpd_send_play(partition_state->conn);
            if (rc == false) {
                MYMPD_LOG_ERROR("Error adding command to command list mpd_send_play");
            }
        }
        if (mpd_command_list_end(partition_state->conn) == true) {
            rc = mpd_response_finish(partition_state->conn);
        }
    }

    if (jukebox_mode != JUKEBOX_OFF) {
        //enable jukebox
        //use the api to persist the setting
        struct t_work_request *request = create_request(-1, 0, MYMPD_API_PLAYER_OPTIONS_SET, NULL);
        request->data = tojson_char(request->data, "jukeboxMode", mpd_client_lookup_jukebox_mode(jukebox_mode), true);
        request->data = tojson_sds(request->data, "jukeboxPlaylist", playlist, false);
        request->data = sdscatlen(request->data, "}}", 2);
        mympd_queue_push(mympd_api_queue, request, 0);
    }
    else {
        //restore old jukebox mode
        partition_state->jukebox_mode = old_jukebox_mode;
    }

    return mympd_check_rc_error_and_recover(partition_state, rc, "command_list");
}

//private functions

/**
 * Timer handler for timer_id TIMER_ID_COVERCACHE_CROP
 */
static void timer_handler_covercache_crop(void) {
    MYMPD_LOG_INFO("Start timer_handler_covercache_crop");
    struct t_work_request *request = create_request(-1, 0, MYMPD_API_COVERCACHE_CROP, NULL);
    request->data = sdscatlen(request->data, "}}", 2);
    mympd_queue_push(mympd_api_queue, request, 0);
}

/**
 * Timer handler for timer_id TIMER_ID_SMARTPLS_UPDATE
 */
static void timer_handler_smartpls_update(void) {
    MYMPD_LOG_INFO("Start timer_handler_smartpls_update");
    struct t_work_request *request = create_request(-1, 0, MYMPD_API_SMARTPLS_UPDATE_ALL, NULL);
    request->data = sdscat(request->data, "\"force\":false}}"); //only update if database has changed
    mympd_queue_push(mympd_api_queue, request, 0);
}

/**
 * Timer handler for timer_id TIMER_ID_CACHES_CREATE
 */
static void timer_handler_caches_create(void) {
    MYMPD_LOG_INFO("Start timer_handler_caches_create");
    struct t_work_request *request = create_request(-1, 0, INTERNAL_API_CACHES_CREATE, NULL);
    request->data = sdscatlen(request->data, "}}", 2);
    mympd_queue_push(mympd_api_queue, request, 0);
}
