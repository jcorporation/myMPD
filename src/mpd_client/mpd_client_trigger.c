/*
 SPDX-License-Identifier: GPL-2.0-or-later
 myMPD (c) 2018-2021 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <string.h>
#include <sys/timerfd.h>
#include <pthread.h>
#include <stdio.h>
#include <stdbool.h>
#include <errno.h>
#include <time.h>
#include <assert.h>
#include <signal.h>
#include <mpd/client.h>

#include "../../dist/src/frozen/frozen.h"
#include "../../dist/src/sds/sds.h"
#include "../sds_extras.h"
#include "../log.h"
#include "../list.h"
#include "config_defs.h"
#include "../api.h"
#include "../tiny_queue.h"
#include "../global.h"
#include "../utility.h"
#include "../mpd_shared/mpd_shared_typedefs.h"
#include "../mpd_shared/mpd_shared_tags.h"
#include "../mpd_shared.h"
#include "mpd_client_utility.h"
#include "mpd_client_trigger.h"

//private definitions
void _trigger_execute(const char *script, struct list *arguments);

static const char *const mpd_trigger_names[] = {
    "mpd_database",
    "mpd_stored_playlist",
    "mpd_playlist",
    "mpd_player",
    "mpd_mixer",
    "mpd_output",
    "mpd_options",
    "mpd_update",
    "mpd_sticker",
    "mpd_subscription",
    "mpd_message",
    "mpd_partition",
    "mpd_neighbor",
    "mpd_mount",
    NULL
};

static const char *const mympd_trigger_names[] = {
    "mympd_scrobble",
    "mympd_start",
    "mympd_stop",
    "mympd_connected",
    "mympd_disconnected",
    NULL
};

//public functions
const char *trigger_name(long event) {
    if (event < 0) {
        for (int i = 0; mympd_trigger_names[i] != NULL; ++i) {
            if (event == (-1 - i)) {
                return mympd_trigger_names[i];
            }
        }
        return NULL;
    }
    for (int i = 0; mpd_trigger_names[i] != NULL; ++i) {
        if (event == (1 << i)) {
            return mpd_trigger_names[i];
        }
    }
    return NULL;
}

sds print_trigger_list(sds buffer) {
    for (int i = 0; mympd_trigger_names[i] != NULL; ++i) {
        buffer = tojson_long(buffer, mympd_trigger_names[i], (-1 -i), true);
    }

    for (int i = 0; mpd_trigger_names[i] != NULL; ++i) {
        if (i > 0) {
            buffer = sdscatlen(buffer, ",", 1);
        }
        buffer = tojson_long(buffer, mpd_trigger_names[i], (1 << i), false);
    }
    return buffer;
}

void trigger_execute(t_mpd_client_state *mpd_client_state, enum trigger_events event) {
    LOG_DEBUG("Trigger event: %s (%d)", trigger_name(event), event);
    struct list_node *current = mpd_client_state->triggers.head;
    while (current != NULL) {
        if (current->value_i == event) {
            LOG_INFO("Executing script %s for trigger %s (%d)", current->value_p, trigger_name(event), event);
            _trigger_execute(current->value_p, (struct list *)current->user_data);
        }
        current = current->next;
    }
}

sds trigger_list(t_mpd_client_state *mpd_client_state, sds buffer, sds method, long request_id) {
    buffer = jsonrpc_result_start(buffer, method, request_id);
    buffer = sdscat(buffer, "\"data\":[");
    int entities_returned = 0;
    struct list_node *current = mpd_client_state->triggers.head;
    int j = 0;
    while (current != NULL) {
        if (entities_returned++) {
            buffer = sdscatlen(buffer, ",", 1);
        }
        buffer = sdscatlen(buffer, "{", 1);
        buffer = tojson_long(buffer, "id", j, true);
        buffer = tojson_char(buffer, "name", current->key, true);
        buffer = tojson_long(buffer, "event", current->value_i, true);
        buffer = tojson_char(buffer, "eventName", trigger_name(current->value_i), true);
        buffer = tojson_char(buffer, "script", current->value_p, true);
        buffer = sdscat(buffer, "\"arguments\": {");
        struct list *arguments = (struct list *)current->user_data;
        struct list_node *argument = arguments->head;
        int i = 0;
        while (argument != NULL) {
            if (i++) {
                buffer = sdscatlen(buffer, ",", 1);
            }
            buffer = tojson_char(buffer, argument->key, argument->value_p, false);
            argument = argument->next;
        }
        buffer = sdscatlen(buffer, "}}", 2);
        current = current->next;
        j++;
    }
    
    buffer = sdscat(buffer, "],");
    buffer = tojson_long(buffer, "returnedEntities", entities_returned, false);
    buffer = jsonrpc_result_end(buffer);
    return buffer;
}

sds trigger_get(t_mpd_client_state *mpd_client_state, sds buffer, sds method, long request_id, int id) {
    struct list_node *current = list_node_at(&mpd_client_state->triggers, id);
    if (current != NULL) {
        buffer = jsonrpc_result_start(buffer, method, request_id);
        buffer = tojson_long(buffer, "id", id, true);
        buffer = tojson_char(buffer, "name", current->key, true);
        buffer = tojson_long(buffer, "event", current->value_i, true);
        buffer = tojson_char(buffer, "script", current->value_p, true);
        buffer = sdscat(buffer, "\"arguments\": {");
        struct list *arguments = (struct list *)current->user_data;
        struct list_node *argument = arguments->head;
        int i = 0;
        while (argument != NULL) {
            if (i++) {
                buffer = sdscatlen(buffer, ",", 1);
            }
            buffer = tojson_char(buffer, argument->key, argument->value_p, false);
            argument = argument->next;
        }
        buffer = sdscatlen(buffer, "}", 1);
        buffer = jsonrpc_result_end(buffer);
    }
    else {
        buffer = jsonrpc_respond_message(buffer, method, request_id, false, "trigger", "warn", "Trigger not found");
    }
    
    return buffer;
}

bool delete_trigger(t_mpd_client_state *mpd_client_state, unsigned idx) {
    struct list_node *toremove = list_node_at(&mpd_client_state->triggers, idx);
    if (toremove != NULL) {
        list_free((struct list *)toremove->user_data);
        return list_shift(&mpd_client_state->triggers, idx);
    }
    return false;
}

void free_trigerlist_arguments(t_mpd_client_state *mpd_client_state) {
    struct list_node *current = mpd_client_state->triggers.head;
    while (current != NULL) {
        list_free((struct list *)current->user_data);
        current = current->next;
    }
}

bool triggerfile_read(t_config *config, t_mpd_client_state *mpd_client_state) {
    sds trigger_file = sdscatfmt(sdsempty(), "%s/state/trigger_list", config->varlibdir);
    char *line = NULL;
    size_t n = 0;
    ssize_t read = 0;
    FILE *fp = fopen(trigger_file, "r");
    if (fp != NULL) {
        while ((read = getline(&line, &n, fp)) > 0) {
            char *name;
            char *script;
            unsigned event;
            int je = json_scanf(line, read, "{name: %Q, event: %u, script: %Q}", &name, &event, &script);
            if (je == 3) {
                struct list *arguments = (struct list *) malloc(sizeof(struct list));
                assert(arguments);
                list_init(arguments);
                void *h = NULL;
                struct json_token key;
                struct json_token val;
                while ((h = json_next_key(line, read, h, ".arguments", &key, &val)) != NULL) {
                    list_push_len(arguments, key.ptr, key.len, 0, val.ptr, val.len, NULL);
                }
                list_push(&mpd_client_state->triggers, name, event, script, arguments);
            }
            FREE_PTR(name);
            FREE_PTR(script);
        }
        FREE_PTR(line);
        fclose(fp);
    }
    else {
        LOG_DEBUG("Can not open file \"%s\": %s", trigger_file, strerror(errno));
    }
    LOG_VERBOSE("Read %d triggers(s) from disc", mpd_client_state->triggers.length);
    sdsfree(trigger_file);
    return true;
}

bool triggerfile_save(t_config *config, t_mpd_client_state *mpd_client_state) {
    if (config->readonly == true) {
        return true;
    }
    LOG_VERBOSE("Saving triggers to disc");
    sds tmp_file = sdscatfmt(sdsempty(), "%s/state/trigger_list.XXXXXX", config->varlibdir);
    int fd = mkstemp(tmp_file);
    if (fd < 0) {
        LOG_ERROR("Can not open file \"%s\" for write: %s", tmp_file, strerror(errno));
        sdsfree(tmp_file);
        return false;
    }
    FILE *fp = fdopen(fd, "w");
    struct list_node *current = mpd_client_state->triggers.head;
    sds buffer = sdsempty();
    while (current != NULL) {
        buffer = sdsreplace(buffer, "{");
        buffer = tojson_char(buffer, "name", current->key, true);
        buffer = tojson_long(buffer, "event", current->value_i, true);
        buffer = tojson_char(buffer, "script", current->value_p, true);
        buffer = sdscat(buffer, "arguments: {");
        struct list *arguments = (struct list *)current->user_data;
        struct list_node *argument = arguments->head;
        int i = 0;
        while (argument != NULL) {
            if (i++) {
                buffer = sdscatlen(buffer, ",", 1);
            }
            buffer = tojson_char(buffer, argument->key, argument->value_p, false);
            argument = argument->next;
        }
        buffer = sdscatlen(buffer, "}}\n", 3);
        fputs(buffer, fp);
        current = current->next;
    }
    fclose(fp);
    sdsfree(buffer);
    sds trigger_file = sdscatfmt(sdsempty(), "%s/state/trigger_list", config->varlibdir);
    if (rename(tmp_file, trigger_file) == -1) {
        LOG_ERROR("Renaming file from %s to %s failed: %s", tmp_file, trigger_file, strerror(errno));
        sdsfree(tmp_file);
        sdsfree(trigger_file);
        return false;
    }
    sdsfree(tmp_file);
    sdsfree(trigger_file);
    return true;    
}

//private functions
void _trigger_execute(const char *script, struct list *arguments) {
    t_work_request *request = create_request(-1, 0, MYMPD_API_SCRIPT_EXECUTE, "MYMPD_API_SCRIPT_EXECUTE", "");
    request->data = sdscat(request->data, "{\"jsonrpc\":\"2.0\",\"id\":0,\"method\":\"MYMPD_API_SCRIPT_EXECUTE\",\"params\":{");
    request->data = tojson_char(request->data, "script", script, true);
    request->data = sdscat(request->data, "arguments: {");
    struct list_node *argument = arguments->head;
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
