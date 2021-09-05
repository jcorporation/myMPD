/*
 SPDX-License-Identifier: GPL-2.0-or-later
 myMPD (c) 2018-2021 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include "mympd_config_defs.h"
#include "mympd_api_timer_handlers.h"

#include "../lib/api.h"
#include "../lib/covercache.h"
#include "../lib/jsonrpc.h"
#include "../lib/log.h"
#include "../lib/mympd_configuration.h"

#include <string.h>

//timer_id 1
void timer_handler_covercache(struct t_timer_definition *definition, void *user_data) {
    MYMPD_LOG_INFO("Start timer_handler_covercache");
    (void) definition;
    struct t_mympd_state *mympd_state = (struct t_mympd_state *) user_data;
    clear_covercache(mympd_state->config->workdir, mympd_state->covercache_keep_days);
}

//timer_id 2
void timer_handler_smartpls_update(struct t_timer_definition *definition, void *user_data) {
    MYMPD_LOG_INFO("Start timer_handler_smartpls_update");
    (void) definition;
    (void) user_data;
    t_work_request *request = create_request(-1, 0, MYMPD_API_SMARTPLS_UPDATE_ALL, NULL);
    request->data = sdscat(request->data, "\"force\":false}}");
    tiny_queue_push(mympd_api_queue, request, 0);
}

void timer_handler_select(struct t_timer_definition *definition, void *user_data) {
    MYMPD_LOG_INFO("Start timer_handler_select for timer \"%s\"", definition->name);
    if (strcmp(definition->action, "player") == 0 && strcmp(definition->subaction, "stopplay") == 0) {
        t_work_request *request = create_request(-1, 0, MYMPD_API_PLAYER_STOP, NULL);
        request->data = sdscat(request->data, "}}");
        tiny_queue_push(mympd_api_queue, request, 0);
    }
    else if (strcmp(definition->action, "player") == 0 && strcmp(definition->subaction, "startplay") == 0) {
        t_work_request *request = create_request(-1, 0, INTERNAL_API_TIMER_STARTPLAY, NULL);
        request->data = tojson_long(request->data, "volume", definition->volume, true);
        request->data = tojson_char(request->data, "playlist", definition->playlist, true);
        request->data = tojson_long(request->data, "jukeboxMode", definition->jukebox_mode, false);
        request->data = sdscat(request->data, "}}");
        tiny_queue_push(mympd_api_queue, request, 0);
    }
    else if (strcmp(definition->action, "script") == 0) {
        t_work_request *request = create_request(-1, 0, MYMPD_API_SCRIPT_EXECUTE, NULL);
        request->data = tojson_char(request->data, "script", definition->subaction, true);
        request->data = sdscat(request->data, "arguments: {");
        struct t_list_node *argument = definition->arguments.head;
        int i = 0;
        while (argument != NULL) {
            if (i++) {
                request->data = sdscatlen(request->data, ",", 1);
            }
            request->data = tojson_char(request->data, argument->key, argument->value_p, false);
            argument = argument->next;
        }
        request->data = sdscat(request->data, "}}}");
        tiny_queue_push(mympd_api_queue, request, 0);
    }
    else {
        MYMPD_LOG_ERROR("Unknown script action: %s - %s", definition->action, definition->subaction);
    }
    (void) user_data;
}
