/*
 SPDX-License-Identifier: GPL-2.0-or-later
 myMPD (c) 2018-2020 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include <stdlib.h>
#include <libgen.h>
#include <pthread.h>
#include <string.h>
#include <inttypes.h>
#include <signal.h>
#include <unistd.h>
#include <mpd/client.h>

#include "../../dist/src/sds/sds.h"
#include "../sds_extras.h"
#include "../../dist/src/frozen/frozen.h"
#include "../mpd_shared.h"
#include "../list.h"
#include "config_defs.h"
#include "../tiny_queue.h"
#include "../api.h"
#include "../global.h"
#include "../utility.h"
#include "../log.h"
#include "mpd_client_utility.h"

void mpd_client_set_timer(enum mympd_cmd_ids cmd_id, const char *cmd, int timeout, int interval, const char *handler) {
    t_work_request *request = create_request(-1, 0, cmd_id, cmd, "");
    request->data = sdscatfmt(request->data, "{\"jsonrpc\":\"2.0\",\"id\":0,\"method\":\"%s\",\"params\":{", cmd);
    request->data = tojson_long(request->data, "timeout", timeout, true);
    request->data = tojson_long(request->data, "interval", interval, true);
    request->data = tojson_char(request->data, "handler", handler, false);
    request->data = sdscat(request->data, "}}");
    tiny_queue_push(mympd_api_queue, request);
}

sds mpd_client_timer_startplay(t_mpd_client_state *mpd_client_state, sds buffer, sds method, int request_id, 
                               unsigned volume, const char *playlist, enum jukebox_modes jukebox_mode) 
{
    //disable jukebox to prevent adding songs to queue from old jukebox queue list
    mpd_client_state->jukebox_mode = JUKEBOX_OFF;
    
    bool rc = false;
    if (mpd_command_list_begin(mpd_client_state->mpd_state->conn, false)) {
        rc = mpd_send_stop(mpd_client_state->mpd_state->conn);
        if (rc == false) {
            LOG_ERROR("Error adding command to command list mpd_send_stop");
        }
        rc = mpd_send_set_volume(mpd_client_state->mpd_state->conn, volume);
        if (rc == false) {
            LOG_ERROR("Error adding command to command list mpd_send_set_volume");
        }
        rc = mpd_send_clear(mpd_client_state->mpd_state->conn);
        if (rc == false) {
            LOG_ERROR("Error adding command to command list mpd_send_clear");
        }
        if (jukebox_mode == JUKEBOX_OFF) {
            rc = mpd_send_load(mpd_client_state->mpd_state->conn, playlist);
            if (rc == false) {
                LOG_ERROR("Error adding command to command list mpd_send_load");
            }
        }
        else {
            rc = mpd_send_consume(mpd_client_state->mpd_state->conn, true);
            if (rc == false) {
                LOG_ERROR("Error adding command to command list mpd_send_consume");
            }
        }
        rc = mpd_send_single(mpd_client_state->mpd_state->conn, false);
        if (rc == false) {
            LOG_ERROR("Error adding command to command list mpd_send_single");
        }
        if (jukebox_mode == JUKEBOX_OFF) {
            rc = mpd_send_play(mpd_client_state->mpd_state->conn);
            if (rc == false) {
                LOG_ERROR("Error adding command to command list mpd_send_play");
            }
        }
        if (mpd_command_list_end(mpd_client_state->mpd_state->conn) == true) {
            rc = mpd_response_finish(mpd_client_state->mpd_state->conn);
        }
    }
        
    t_work_request *request = create_request(-1, 0, MYMPD_API_SETTINGS_SET, "MYMPD_API_SETTINGS_SET", "");
    request->data = sdscat(request->data, "{\"jsonrpc\":\"2.0\",\"id\":0,\"method\":\"MYMPD_API_SETTINGS_SET\",\"params\":{");
    request->data = tojson_long(request->data, "jukeboxMode", jukebox_mode, true);
    
    if (jukebox_mode != JUKEBOX_OFF) {
        request->data = tojson_char(request->data, "jukeboxPlaylist", playlist, false);
    }
    request->data = sdscat(request->data, "}}");
    tiny_queue_push(mympd_api_queue, request);

    buffer = respond_with_mpd_error_or_ok(mpd_client_state->mpd_state, buffer, method, request_id, rc, "mpd_client_timer_startplay");
    return buffer;
}
