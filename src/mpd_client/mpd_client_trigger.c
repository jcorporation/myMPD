/*
 SPDX-License-Identifier: GPL-2.0-or-later
 myMPD (c) 2018-2021 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include "mympd_config_defs.h"
#include "mpd_client_trigger.h"

#include "../../dist/src/frozen/frozen.h"
#include "../api.h"
#include "../global.h"
#include "../lib/jsonrpc.h"
#include "../lib/log.h"
#include "../lib/sds_extras.h"
#include "../utility.h"

#include <assert.h>
#include <errno.h>
#include <stdlib.h>

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

void trigger_execute(struct t_mympd_state *mympd_state, enum trigger_events event) {
    MYMPD_LOG_DEBUG("Trigger event: %s (%d)", trigger_name(event), event);
    struct list_node *current = mympd_state->triggers.head;
    while (current != NULL) {
        if (current->value_i == event) {
            MYMPD_LOG_NOTICE("Executing script %s for trigger %s (%d)", current->value_p, trigger_name(event), event);
            _trigger_execute(current->value_p, (struct list *)current->user_data);
        }
        current = current->next;
    }
}

sds trigger_list(struct t_mympd_state *mympd_state, sds buffer, sds method, long request_id) {
    buffer = jsonrpc_result_start(buffer, method, request_id);
    buffer = sdscat(buffer, "\"data\":[");
    int entities_returned = 0;
    struct list_node *current = mympd_state->triggers.head;
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

sds trigger_get(struct t_mympd_state *mympd_state, sds buffer, sds method, long request_id, int id) {
    struct list_node *current = list_node_at(&mympd_state->triggers, id);
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

bool delete_trigger(struct t_mympd_state *mympd_state, unsigned idx) {
    struct list_node *toremove = list_node_at(&mympd_state->triggers, idx);
    if (toremove != NULL) {
        list_free((struct list *)toremove->user_data);
        return list_shift(&mympd_state->triggers, idx);
    }
    return false;
}

void free_trigerlist_arguments(struct t_mympd_state *mympd_state) {
    struct list_node *current = mympd_state->triggers.head;
    while (current != NULL) {
        list_free((struct list *)current->user_data);
        current = current->next;
    }
}

bool triggerfile_read(struct t_mympd_state *mympd_state) {
    sds trigger_file = sdscatfmt(sdsempty(), "%s/state/trigger_list", mympd_state->config->workdir);
    char *line = NULL;
    size_t n = 0;
    ssize_t read = 0;
    errno = 0;
    FILE *fp = fopen(trigger_file, "r");
    if (fp != NULL) {
        while ((read = getline(&line, &n, fp)) > 0) {
            char *name;
            char *script;
            int event;
            int je = json_scanf(line, read, "{name: %Q, event: %d, script: %Q}", &name, &event, &script);
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
                list_push(&mympd_state->triggers, name, event, script, arguments);
            }
            FREE_PTR(name);
            FREE_PTR(script);
        }
        FREE_PTR(line);
        fclose(fp);
    }
    else {
        MYMPD_LOG_DEBUG("Can not open file \"%s\"", trigger_file);
        if (errno != ENOENT) {
            MYMPD_LOG_ERRNO(errno);
        }
    }
    MYMPD_LOG_INFO("Read %d triggers(s) from disc", mympd_state->triggers.length);
    sdsfree(trigger_file);
    return true;
}

bool triggerfile_save(struct t_mympd_state *mympd_state) {
    MYMPD_LOG_INFO("Saving triggers to disc");
    sds tmp_file = sdscatfmt(sdsempty(), "%s/state/trigger_list.XXXXXX", mympd_state->config->workdir);
    errno = 0;
    int fd = mkstemp(tmp_file);
    if (fd < 0) {
        MYMPD_LOG_ERROR("Can not open file \"%s\" for write", tmp_file);
        MYMPD_LOG_ERRNO(errno);
        sdsfree(tmp_file);
        return false;
    }
    FILE *fp = fdopen(fd, "w");
    struct list_node *current = mympd_state->triggers.head;
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
    sds trigger_file = sdscatfmt(sdsempty(), "%s/state/trigger_list", mympd_state->config->workdir);
    errno = 0;
    if (rename(tmp_file, trigger_file) == -1) {
        MYMPD_LOG_ERROR("Renaming file from %s to %s failed", tmp_file, trigger_file);
        MYMPD_LOG_ERRNO(errno);
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
    t_work_request *request = create_request(-1, 0, MYMPD_API_SCRIPT_EXECUTE, NULL);
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
