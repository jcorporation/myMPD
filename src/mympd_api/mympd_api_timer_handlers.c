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
#include "../lib/mympd_configuration.h"
#include "../lib/sds_extras.h"
#include "../mpd_client/mpd_client_jukebox.h"
#include "../mpd_client/mpd_client_volume.h"
#include "../mpd_shared.h"
#include "mympd_api_utility.h"

#include <string.h>

//timer_id 1
void timer_handler_covercache(struct t_timer_definition *definition, void *user_data) {
    MYMPD_LOG_INFO("Start timer_handler_covercache");
    (void) definition;
    struct t_mympd_state *mympd_state = (struct t_mympd_state *) user_data;
    covercache_clear(mympd_state->config->cachedir, mympd_state->covercache_keep_days);
}

//timer_id 2
void timer_handler_smartpls_update(struct t_timer_definition *definition, void *user_data) {
    MYMPD_LOG_INFO("Start timer_handler_smartpls_update");
    (void) definition;
    (void) user_data;
    struct t_work_request *request = create_request(-1, 0, MYMPD_API_SMARTPLS_UPDATE_ALL, NULL);
    request->data = sdscat(request->data, "\"force\":false}}");
    mympd_queue_push(mympd_api_queue, request, 0);
}

void timer_handler_select(struct t_timer_definition *definition, void *user_data) {
    MYMPD_LOG_INFO("Start timer_handler_select for timer \"%s\"", definition->name);
    if (strcmp(definition->action, "player") == 0 && strcmp(definition->subaction, "stopplay") == 0) {
        struct t_work_request *request = create_request(-1, 0, MYMPD_API_PLAYER_STOP, NULL);
        request->data = sdscatlen(request->data, "}}", 2);
        mympd_queue_push(mympd_api_queue, request, 0);
    }
    else if (strcmp(definition->action, "player") == 0 && strcmp(definition->subaction, "startplay") == 0) {
        struct t_work_request *request = create_request(-1, 0, INTERNAL_API_TIMER_STARTPLAY, NULL);
        request->data = tojson_uint(request->data, "volume", definition->volume, true);
        request->data = tojson_char(request->data, "plist", definition->playlist, true);
        request->data = tojson_long(request->data, "jukeboxMode", definition->jukebox_mode, false);
        request->data = sdscatlen(request->data, "}}", 2);
        mympd_queue_push(mympd_api_queue, request, 0);
    }
    else if (strcmp(definition->action, "script") == 0) {
        struct t_work_request *request = create_request(-1, 0, MYMPD_API_SCRIPT_EXECUTE, NULL);
        request->data = tojson_char(request->data, "script", definition->subaction, true);
        request->data = sdscat(request->data, "\"arguments\":{");
        struct t_list_node *argument = definition->arguments.head;
        int i = 0;
        while (argument != NULL) {
            if (i++) {
                request->data = sdscatlen(request->data, ",", 1);
            }
            request->data = tojson_char(request->data, argument->key, argument->value_p, false);
            argument = argument->next;
        }
        request->data = sdscatlen(request->data, "}}}", 3);
        mympd_queue_push(mympd_api_queue, request, 0);
    }
    else {
        MYMPD_LOG_ERROR("Unknown script action: %s - %s", definition->action, definition->subaction);
    }
    (void) user_data;
}

sds mympd_api_timer_startplay(struct t_mympd_state *mympd_state, sds buffer, sds method, long request_id,
                               unsigned volume, const char *playlist, enum jukebox_modes jukebox_mode)
{
    //disable jukebox to prevent adding songs to queue from old jukebox queue list
    enum jukebox_modes old_jukebox_mode = mympd_state->jukebox_mode;
    mympd_state->jukebox_mode = JUKEBOX_OFF;

    int old_volume = mpd_client_get_volume(mympd_state->mpd_state);

    bool rc = false;
    if (mpd_command_list_begin(mympd_state->mpd_state->conn, false)) {
        rc = mpd_send_stop(mympd_state->mpd_state->conn);
        if (rc == false) {
            MYMPD_LOG_ERROR("Error adding command to command list mpd_send_stop");
        }
        if (old_volume != -1) {
            rc = mpd_send_set_volume(mympd_state->mpd_state->conn, volume);
            if (rc == false) {
                MYMPD_LOG_ERROR("Error adding command to command list mpd_send_set_volume");
            }
        }
        rc = mpd_send_clear(mympd_state->mpd_state->conn);
        if (rc == false) {
            MYMPD_LOG_ERROR("Error adding command to command list mpd_send_clear");
        }
        if (jukebox_mode == JUKEBOX_OFF) {
            rc = mpd_send_load(mympd_state->mpd_state->conn, playlist);
            if (rc == false) {
                MYMPD_LOG_ERROR("Error adding command to command list mpd_send_load");
            }
        }
        else {
            rc = mpd_send_consume(mympd_state->mpd_state->conn, true);
            if (rc == false) {
                MYMPD_LOG_ERROR("Error adding command to command list mpd_send_consume");
            }
        }
        rc = mpd_send_single_state(mympd_state->mpd_state->conn, MPD_SINGLE_OFF);
        if (rc == false) {
            MYMPD_LOG_ERROR("Error adding command to command list mpd_send_single");
        }
        if (jukebox_mode == JUKEBOX_OFF) {
            rc = mpd_send_play(mympd_state->mpd_state->conn);
            if (rc == false) {
                MYMPD_LOG_ERROR("Error adding command to command list mpd_send_play");
            }
        }
        if (mpd_command_list_end(mympd_state->mpd_state->conn) == true) {
            rc = mpd_response_finish(mympd_state->mpd_state->conn);
        }
    }

    if (jukebox_mode != JUKEBOX_OFF) {
        //enable jukebox
        //use the api to persist the setting
        struct t_work_request *request = create_request(-1, 0, MYMPD_API_PLAYER_OPTIONS_SET, NULL);
        request->data = tojson_char(request->data, "jukeboxMode", mympd_lookup_jukebox_mode(jukebox_mode), true);
        request->data = tojson_char(request->data, "jukeboxPlaylist", playlist, false);
        request->data = sdscatlen(request->data, "}}", 2);
        mympd_queue_push(mympd_api_queue, request, 0);
    }
    else {
        //restore old jukebox mode
        mympd_state->jukebox_mode = old_jukebox_mode;
    }

    bool result;
    buffer = respond_with_mpd_error_or_ok(mympd_state->mpd_state, buffer, method, request_id, rc, "mympd_api_timer_startplay", &result);
    return buffer;
}
