/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2022 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include "mympd_config_defs.h"
#include "mympd_api_trigger.h"

#include "../lib/filehandler.h"
#include "../lib/api.h"
#include "../lib/jsonrpc.h"
#include "../lib/log.h"
#include "../lib/mem.h"
#include "../lib/sds_extras.h"

#include <errno.h>
#include <stdlib.h>
#include <unistd.h>

//private definitions
void _trigger_execute(sds script, struct t_list *arguments);

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
    "mympd_feedback",
    NULL
};

//public functions
const char *mympd_api_trigger_name(long event) {
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

sds mympd_api_trigger_print_trigger_list(sds buffer) {
    for (int i = 0; mympd_trigger_names[i] != NULL; ++i) {
        buffer = tojson_long(buffer, mympd_trigger_names[i], (-1 - i), true);
    }

    for (int i = 0; mpd_trigger_names[i] != NULL; ++i) {
        if (i > 0) {
            buffer = sdscatlen(buffer, ",", 1);
        }
        buffer = tojson_long(buffer, mpd_trigger_names[i], (1 << i), false);
    }
    return buffer;
}

void mympd_api_trigger_execute(struct t_list *trigger_list, enum trigger_events event) {
    MYMPD_LOG_DEBUG("Trigger event: %s (%d)", mympd_api_trigger_name(event), event);
    struct t_list_node *current = trigger_list->head;
    while (current != NULL) {
        if (current->value_i == event) {
            MYMPD_LOG_NOTICE("Executing script \"%s\" for trigger \"%s\" (%d)", current->value_p,
                mympd_api_trigger_name(event), event);
            _trigger_execute(current->value_p, (struct t_list *)current->user_data);
        }
        current = current->next;
    }
}

void mympd_api_trigger_execute_feedback(struct t_list *trigger_list, sds uri, int vote) {
    MYMPD_LOG_DEBUG("Trigger event: mympd_feedback (-6) for \"%s\", vote %d", uri, vote);
    //trigger mympd_feedback executes scripts with uri and vote arguments
    struct t_list script_arguments;
    list_init(&script_arguments);
    list_push(&script_arguments, "uri", 0, uri, NULL);
    const char *vote_str = vote == 0 ? "0" :
                           vote == 1 ? "1" : "2";
    list_push(&script_arguments, "vote", 0, vote_str, NULL);
    struct t_list_node *current = trigger_list->head;
    while (current != NULL) {
        if (current->value_i == TRIGGER_MYMPD_FEEDBACK) {
            MYMPD_LOG_NOTICE("Executing script \"%s\" for trigger \"mympd_feedback\" (-6)", current->value_p);
            _trigger_execute(current->value_p, &script_arguments);
        }
        current = current->next;
    }
    list_clear(&script_arguments);
}

sds mympd_api_trigger_list(struct t_list *trigger_list, sds buffer, sds method, long request_id) {
    buffer = jsonrpc_result_start(buffer, method, request_id);
    buffer = sdscat(buffer, "\"data\":[");
    int entities_returned = 0;
    struct t_list_node *current = trigger_list->head;
    int j = 0;
    while (current != NULL) {
        if (entities_returned++) {
            buffer = sdscatlen(buffer, ",", 1);
        }
        buffer = sdscatlen(buffer, "{", 1);
        buffer = tojson_int(buffer, "id", j, true);
        buffer = tojson_sds(buffer, "name", current->key, true);
        buffer = tojson_llong(buffer, "event", current->value_i, true);
        buffer = tojson_char(buffer, "eventName", mympd_api_trigger_name((long)current->value_i), true);
        buffer = tojson_sds(buffer, "script", current->value_p, true);
        buffer = sdscat(buffer, "\"arguments\": {");
        struct t_list *arguments = (struct t_list *)current->user_data;
        struct t_list_node *argument = arguments->head;
        int i = 0;
        while (argument != NULL) {
            if (i++) {
                buffer = sdscatlen(buffer, ",", 1);
            }
            buffer = tojson_sds(buffer, argument->key, argument->value_p, false);
            argument = argument->next;
        }
        buffer = sdscatlen(buffer, "}}", 2);
        current = current->next;
        j++;
    }

    buffer = sdscatlen(buffer, "],", 2);
    buffer = tojson_long(buffer, "returnedEntities", entities_returned, false);
    buffer = jsonrpc_result_end(buffer);
    return buffer;
}

sds mympd_api_trigger_get(struct t_list *trigger_list, sds buffer, sds method, long request_id, long id) {
    struct t_list_node *current = list_node_at(trigger_list, id);
    if (current != NULL) {
        buffer = jsonrpc_result_start(buffer, method, request_id);
        buffer = tojson_long(buffer, "id", id, true);
        buffer = tojson_sds(buffer, "name", current->key, true);
        buffer = tojson_llong(buffer, "event", current->value_i, true);
        buffer = tojson_sds(buffer, "script", current->value_p, true);
        buffer = sdscat(buffer, "\"arguments\": {");
        struct t_list *arguments = (struct t_list *)current->user_data;
        struct t_list_node *argument = arguments->head;
        int i = 0;
        while (argument != NULL) {
            if (i++) {
                buffer = sdscatlen(buffer, ",", 1);
            }
            buffer = tojson_sds(buffer, argument->key, argument->value_p, false);
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

bool mympd_api_trigger_delete(struct t_list *trigger_list, long idx) {
    struct t_list_node *to_remove = list_node_extract(trigger_list, idx);
    if (to_remove != NULL) {
        list_node_free_user_data(to_remove, list_free_cb_t_list_user_data);
        return true;
    }
    return false;
}

bool mympd_api_trigger_file_read(struct t_list *trigger_list, sds workdir) {
    sds trigger_file = sdscatfmt(sdsempty(), "%S/state/trigger_list", workdir);
    errno = 0;
    FILE *fp = fopen(trigger_file, OPEN_FLAGS_READ);
    if (fp == NULL) {
        MYMPD_LOG_DEBUG("Can not open file \"%s\"", trigger_file);
        if (errno != ENOENT) {
            MYMPD_LOG_ERRNO(errno);
        }
        FREE_SDS(trigger_file);
        return false;
    }
    int i = 0;
    sds line = sdsempty();
    while (sds_getline(&line, fp, LINE_LENGTH_MAX) == 0) {
        if (i > LIST_TRIGGER_MAX) {
            MYMPD_LOG_WARN("Too many triggers defined");
            break;
        }
        if (validate_json(line) == false) {
            MYMPD_LOG_ERROR("Invalid line");
            break;
        }
        sds name = NULL;
        sds script = NULL;
        int event;
        struct t_list *arguments = list_new();
        if (json_get_string(line, "$.name", 1, FILENAME_LEN_MAX, &name, vcb_isfilename, NULL) == true &&
            json_get_string(line, "$.script", 0, FILENAME_LEN_MAX, &script, vcb_isfilename, NULL) == true &&
            json_get_int_max(line, "$.event", &event, NULL) == true &&
            json_get_object_string(line, "$.arguments", arguments, vcb_isname, 10, NULL))
        {
            list_push(trigger_list, name, event, script, arguments);
        }
        else {
            list_free(arguments);
        }
        FREE_SDS(name);
        FREE_SDS(script);
        i++;
    }
    FREE_SDS(line);
    (void) fclose(fp);
    MYMPD_LOG_INFO("Read %ld triggers(s) from disc", trigger_list->length);
    FREE_SDS(trigger_file);
    return true;
}

static sds trigger_to_line_cb(sds buffer, struct t_list_node *current) {
    buffer = sdscatlen(buffer, "{", 1);
    buffer = tojson_sds(buffer, "name", current->key, true);
    buffer = tojson_llong(buffer, "event", current->value_i, true);
    buffer = tojson_sds(buffer, "script", current->value_p, true);
    buffer = sdscat(buffer, "\"arguments\":{");
    struct t_list *arguments = (struct t_list *)current->user_data;
    struct t_list_node *argument = arguments->head;
    int i = 0;
    while (argument != NULL) {
        if (i++) {
            buffer = sdscatlen(buffer, ",", 1);
        }
        buffer = tojson_sds(buffer, argument->key, argument->value_p, false);
        argument = argument->next;
    }
    buffer = sdscatlen(buffer, "}}\n", 3);
    return buffer;
}

bool mympd_api_trigger_file_save(struct t_list *trigger_list, sds workdir) {
    MYMPD_LOG_INFO("Saving triggers to disc");
    sds filepath = sdscatfmt(sdsempty(), "%S/state/trigger_list", workdir);
    bool rc = list_write_to_disk(filepath, trigger_list, trigger_to_line_cb);
    FREE_SDS(filepath);
    return rc;
}

//private functions
void _trigger_execute(sds script, struct t_list *arguments) {
    struct t_work_request *request = create_request(-1, 0, MYMPD_API_SCRIPT_EXECUTE, NULL);
    request->data = tojson_sds(request->data, "script", script, true);
    request->data = sdscat(request->data, "\"arguments\": {");
    struct t_list_node *argument = arguments->head;
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
