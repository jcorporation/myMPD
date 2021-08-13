/*
 SPDX-License-Identifier: GPL-2.0-or-later
 myMPD (c) 2018-2021 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include "mympd_config_defs.h"
#include "mpd_client_timer.h"

#include "../lib/jsonrpc.h"
#include "../lib/log.h"
#include "../mpd_shared.h"

void mpd_client_set_timer(enum mympd_cmd_ids cmd_id, int timeout, int interval, const char *handler) {
    t_work_request *request = create_request(-1, 0, cmd_id, NULL);
    request->data = tojson_long(request->data, "timeout", timeout, true);
    request->data = tojson_long(request->data, "interval", interval, true);
    request->data = tojson_char(request->data, "handler", handler, false);
    request->data = sdscat(request->data, "}}");
    tiny_queue_push(mympd_api_queue, request, 0);
}

sds mpd_client_timer_startplay(struct t_mympd_state *mympd_state, sds buffer, sds method, long request_id, 
                               unsigned volume, const char *playlist, enum jukebox_modes jukebox_mode) 
{
    //disable jukebox to prevent adding songs to queue from old jukebox queue list
    mympd_state->jukebox_mode = JUKEBOX_OFF;
    
    bool rc = false;
    if (mpd_command_list_begin(mympd_state->mpd_state->conn, false)) {
        rc = mpd_send_stop(mympd_state->mpd_state->conn);
        if (rc == false) {
            MYMPD_LOG_ERROR("Error adding command to command list mpd_send_stop");
        }
        rc = mpd_send_set_volume(mympd_state->mpd_state->conn, volume);
        if (rc == false) {
            MYMPD_LOG_ERROR("Error adding command to command list mpd_send_set_volume");
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
        rc = mpd_send_single(mympd_state->mpd_state->conn, false);
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
        
    t_work_request *request = create_request(-1, 0, MYMPD_API_SETTINGS_SET, NULL);
    request->data = tojson_long(request->data, "jukeboxMode", jukebox_mode, true);
    
    if (jukebox_mode != JUKEBOX_OFF) {
        request->data = tojson_char(request->data, "jukeboxPlaylist", playlist, false);
    }
    request->data = sdscat(request->data, "}}");
    tiny_queue_push(mympd_api_queue, request, 0);

    buffer = respond_with_mpd_error_or_ok(mympd_state->mpd_state, buffer, method, request_id, rc, "mpd_client_timer_startplay");
    return buffer;
}
