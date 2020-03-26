/*
 SPDX-License-Identifier: GPL-2.0-or-later
 myMPD (c) 2018-2020 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include <stdbool.h>
#include <stdlib.h>
#include <signal.h>
#include <time.h>
#include <string.h>

#include "../../dist/src/sds/sds.h"
#include "../log.h"
#include "../list.h"
#include "../tiny_queue.h"
#include "../api.h"
#include "../global.h"
#include "config_defs.h"
#include "../utility.h"
#include "../maintenance.h"
#include "mympd_api_utility.h"
#include "mympd_api_timer.h"
#include "mympd_api_timer_handlers.h"

//timer_id 1
void timer_handler_covercache(struct t_timer_definition *definition, void *user_data) {
    LOG_VERBOSE("Start timer_handler_covercache");
    (void) definition;
    t_config *config = (t_config *) user_data;
    clear_covercache(config, -1);
}

//timer_id 2
void timer_handler_smartpls_update(struct t_timer_definition *definition, void *user_data) {
    LOG_VERBOSE("Start timer_handler_smartpls_update");
    (void) definition;
    (void) user_data;
    t_work_request *request = create_request(-1, 0, MPD_API_SMARTPLS_UPDATE_ALL, "MPD_API_SMARTPLS_UPDATE_ALL", "");
    request->data = sdscat(request->data, "{\"jsonrpc\":\"2.0\",\"id\":0,\"method\":\"MPD_API_SMARTPLS_UPDATE_ALL\",\"params\":{}}");
    tiny_queue_push(mpd_client_queue, request);
}

void timer_handler_select(struct t_timer_definition *definition, void *user_data) {
    if (definition->enabled == false) {
        //timer not enabled
        return;
    }
    else {
        time_t now = time(NULL);
        struct tm tms;
        localtime_r(&now, &tms);
        tms.tm_wday++;
        if (definition->weekdays[tms.tm_wday] == false) {
            return;
        }
    }
    LOG_VERBOSE("Start timer_handler_select for timer \"%s\"", definition->name);
    if (strcmp(definition->action, "stopplay") == 0) {
        t_work_request *request = create_request(-1, 0, MPD_API_PLAYER_STOP, "MPD_API_PLAYER_STOP", "");
        request->data = sdscat(request->data, "{\"jsonrpc\":\"2.0\",\"id\":0,\"method\":\"MPD_API_PLAYER_STOP\",\"params\":{}}");
        tiny_queue_push(mpd_client_queue, request);
    }
    else if (strcmp(definition->action, "startplay") == 0) {
        t_work_request *request = create_request(-1, 0, MPD_API_TIMER_STARTPLAY, "MPD_API_TIMER_STARTPLAY", "");
        request->data = sdscat(request->data, "{\"jsonrpc\":\"2.0\",\"id\":0,\"method\":\"MPD_API_TIMER_STARTPLAY\",\"params\":{");
        request->data = tojson_long(request->data, "volume", definition->volume, true);
        request->data = tojson_char(request->data, "playlist", definition->playlist, true);
        request->data = tojson_long(request->data, "jukeboxMode", definition->jukebox_mode, true);
        request->data = sdscat(request->data, "}}");
        tiny_queue_push(mpd_client_queue, request);        
    }
    else {
        t_work_request *request = create_request(-1, 0, MYMPD_API_SYSCMD, "MYMPD_API_SYSCMD", "");
        request->data = sdscat(request->data, "{\"jsonrpc\":\"2.0\",\"id\":0,\"method\":\"MYMPD_API_SYSCMD\",\"params\":{");
        request->data = tojson_char(request->data, "cmd", definition->action, false);
        request->data = sdscat(request->data, "}}");
        tiny_queue_push(mympd_api_queue, request);
    }
    (void) user_data;
}
