/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2023 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include "compile_time.h"
#include "src/mympd_api/trigger.h"

#include "src/lib/api.h"
#include "src/lib/filehandler.h"
#include "src/lib/jsonrpc.h"
#include "src/lib/log.h"
#include "src/lib/mem.h"
#include "src/lib/msg_queue.h"
#include "src/lib/sds_extras.h"
#include "src/lib/state_files.h"

#include <errno.h>
#include <string.h>

/**
 * Private definitions
 */

static void list_free_cb_trigger_data(struct t_list_node *current);
static sds trigger_to_line_cb(sds buffer, struct t_list_node *current);
void trigger_execute(sds script, struct t_list *arguments, const char *partition);

/**
 * All MPD idle events
 */
static const char *const mpd_event_names[] = {
    "mpd_database",
    "mpd_stored_playlist",
    "mpd_queue",
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

/**
 * MPD idle events for triggers
 * This events are enabled by the idle bitmask
 */
static const char *const trigger_event_names[] = {
    "mpd_database",
    "mpd_stored_playlist",
    "mpd_queue",
    "mpd_player",
    "mpd_mixer",
    "mpd_output",
    "mpd_options",
    "mpd_partition",
    NULL
};

/**
 * myMPD events for triggers
 */
static const char *const mympd_event_names[] = {
    "mympd_scrobble",
    "mympd_start",
    "mympd_stop",
    "mympd_connected",
    "mympd_disconnected",
    "mympd_feedback",
    NULL
};

/**
 * Public functions
 */

/**
 * Returns the event name
 * @param event event to resolv
 * @return trigger as string
 */
const char *mympd_api_event_name(long event) {
    if (event < 0) {
        for (int i = 0; mympd_event_names[i] != NULL; ++i) {
            if (event == (-1 - i)) {
                return mympd_event_names[i];
            }
        }
        return NULL;
    }
    for (int i = 0; mpd_event_names[i] != NULL; ++i) {
        if (event == (1 << i)) {
            return mpd_event_names[i];
        }
    }
    return NULL;
}

/**
 * Prints all events names as json string
 * @param buffer already allocated sds string to append the response
 * @return pointer to buffer
 */
sds mympd_api_trigger_print_event_list(sds buffer) {
    for (int i = 0; mympd_event_names[i] != NULL; ++i) {
        buffer = tojson_long(buffer, mympd_event_names[i], (-1 - i), true);
    }

    for (int i = 0; trigger_event_names[i] != NULL; ++i) {
        if (i > 0) {
            buffer = sdscatlen(buffer, ",", 1);
        }
        buffer = tojson_long(buffer, trigger_event_names[i], (1 << i), false);
    }
    return buffer;
}

/**
 * Executes all scripts associated with the trigger
 * @param trigger_list trigger list
 * @param event trigger to execute scripts for
 * @param partition mpd partition
 */
void mympd_api_trigger_execute(struct t_list *trigger_list, enum trigger_events event, const char *partition) {
    MYMPD_LOG_DEBUG("\"%s\": Trigger event: %s (%d)", partition, mympd_api_event_name(event), event);
    struct t_list_node *current = trigger_list->head;
    while (current != NULL) {
        if (current->value_i == event &&
                (strcmp(partition, current->value_p) == 0 ||
                 strcmp(current->value_p, MPD_PARTITION_ALL) == 0)
           )
        {
            struct t_trigger_data *trigger_data = (struct t_trigger_data *)current->user_data;
            MYMPD_LOG_NOTICE("\"%s\": Executing script \"%s\" for trigger \"%s\" (%d)",
                partition, trigger_data->script, mympd_api_event_name(event), event);
            trigger_execute(trigger_data->script, &trigger_data->arguments, partition);
        }
        current = current->next;
    }
}

/**
 * Executes the feedback timer
 * @param trigger_list trigger list
 * @param uri feedback uri
 * @param vote the feedback
 * @param partition mpd partition
 */
void mympd_api_trigger_execute_feedback(struct t_list *trigger_list, sds uri, int vote, const char *partition) {
    MYMPD_LOG_DEBUG("\"%s\": Trigger event: mympd_feedback (-6) for \"%s\", vote %d", partition, uri, vote);
    //trigger mympd_feedback executes scripts with uri and vote arguments
    struct t_list script_arguments;
    list_init(&script_arguments);
    list_push(&script_arguments, "uri", 0, uri, NULL);
    const char *vote_str = vote == 0 ? "0" :
                           vote == 1 ? "1" : "2";
    list_push(&script_arguments, "vote", 0, vote_str, NULL);
    struct t_list_node *current = trigger_list->head;
    while (current != NULL) {
        if (current->value_i == TRIGGER_MYMPD_FEEDBACK &&
                (strcmp(partition, current->value_p) == 0 ||
                 strcmp(current->value_p, MPD_PARTITION_ALL) == 0)
           )
        {
            MYMPD_LOG_NOTICE("Executing script \"%s\" for trigger \"mympd_feedback\" (-6)", current->value_p);
            struct t_trigger_data *trigger_data = (struct t_trigger_data *)current->user_data;
            trigger_execute(trigger_data->script, &script_arguments, partition);
        }
        current = current->next;
    }
    list_clear(&script_arguments);
}

/**
 * Prints the trigger list as jsonrpc response
 * @param trigger_list trigger list
 * @param buffer already allocated sds string to append the response
 * @param request_id jsonrpc request id
 * @param partition mpd partition
 * @return pointer to buffer
 */
sds mympd_api_trigger_list(struct t_list *trigger_list, sds buffer, long request_id, const char *partition) {
    enum mympd_cmd_ids cmd_id = MYMPD_API_TRIGGER_GET;
    buffer = jsonrpc_respond_start(buffer, cmd_id, request_id);
    buffer = sdscat(buffer, "\"data\":[");
    int entities_returned = 0;
    struct t_list_node *current = trigger_list->head;
    int j = 0;
    while (current != NULL) {
        if (strcmp(partition, current->value_p) == 0 ||
            strcmp(current->value_p, MPD_PARTITION_ALL) == 0)
        {
            if (entities_returned++) {
                buffer = sdscatlen(buffer, ",", 1);
            }
            struct t_trigger_data *trigger_data = (struct t_trigger_data *)current->user_data;
            buffer = sdscatlen(buffer, "{", 1);
            buffer = tojson_int(buffer, "id", j, true);
            buffer = tojson_sds(buffer, "name", current->key, true);
            buffer = tojson_llong(buffer, "event", current->value_i, true);
            buffer = tojson_char(buffer, "eventName", mympd_api_event_name((long)current->value_i), true);
            buffer = tojson_sds(buffer, "partition", current->value_p, true);
            buffer = tojson_sds(buffer, "script", trigger_data->script, true);
            buffer = sdscat(buffer, "\"arguments\": {");
            struct t_list_node *argument = trigger_data->arguments.head;
            int i = 0;
            while (argument != NULL) {
                if (i++) {
                    buffer = sdscatlen(buffer, ",", 1);
                }
                buffer = tojson_sds(buffer, argument->key, argument->value_p, false);
                argument = argument->next;
            }
            buffer = sdscatlen(buffer, "}}", 2);
        }
        current = current->next;
        j++;
    }

    buffer = sdscatlen(buffer, "],", 2);
    buffer = tojson_long(buffer, "returnedEntities", entities_returned, false);
    buffer = jsonrpc_end(buffer);
    return buffer;
}

/**
 * Prints the trigger with given id as jsonrpc response
 * @param trigger_list trigger list
 * @param buffer already allocated sds string to append the response
 * @param request_id jsonrpc request id
 * @param id trigger id to print
 * @return pointer to buffer
 */
sds mympd_api_trigger_get(struct t_list *trigger_list, sds buffer, long request_id, long id) {
    enum mympd_cmd_ids cmd_id = MYMPD_API_TRIGGER_GET;
    struct t_list_node *current = list_node_at(trigger_list, id);
    if (current != NULL) {
        struct t_trigger_data *trigger_data = (struct t_trigger_data *)current->user_data;
        buffer = jsonrpc_respond_start(buffer, cmd_id, request_id);
        buffer = tojson_long(buffer, "id", id, true);
        buffer = tojson_sds(buffer, "name", current->key, true);
        buffer = tojson_llong(buffer, "event", current->value_i, true);
        buffer = tojson_sds(buffer, "partition", current->value_p, true);
        buffer = tojson_sds(buffer, "script", trigger_data->script, true);
        buffer = sdscat(buffer, "\"arguments\": {");
        struct t_list_node *argument = trigger_data->arguments.head;
        int i = 0;
        while (argument != NULL) {
            if (i++) {
                buffer = sdscatlen(buffer, ",", 1);
            }
            buffer = tojson_sds(buffer, argument->key, argument->value_p, false);
            argument = argument->next;
        }
        buffer = sdscatlen(buffer, "}", 1);
        buffer = jsonrpc_end(buffer);
    }
    else {
        buffer = jsonrpc_respond_message(buffer, cmd_id, request_id,
            JSONRPC_FACILITY_TRIGGER, JSONRPC_SEVERITY_WARN, "Trigger not found");
    }

    return buffer;
}

/**
 * Deletes a trigger
 * @param trigger_list trigger list
 * @param idx index of trigger node to remove
 * @return true on success, else false
 */
bool mympd_api_trigger_delete(struct t_list *trigger_list, long idx) {
    struct t_list_node *to_remove = list_node_extract(trigger_list, idx);
    if (to_remove != NULL) {
        list_node_free_user_data(to_remove, list_free_cb_trigger_data);
        return true;
    }
    return false;
}

/**
 * Reads the trigger file from disc and populates the trigger list
 * @param trigger_list trigger list
 * @param workdir working directory
 * @return true on success, else false
 */
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
    while (sds_getline(&line, fp, LINE_LENGTH_MAX) >= 0) {
        if (i > LIST_TRIGGER_MAX) {
            MYMPD_LOG_WARN("Too many triggers defined");
            break;
        }
        if (validate_json_object(line) == false) {
            MYMPD_LOG_ERROR("Invalid line");
            break;
        }
        sds name = NULL;
        sds partition = NULL;
        int event;
        struct t_trigger_data *trigger_data = trigger_data_new();
        if (json_get_string(line, "$.name", 1, FILENAME_LEN_MAX, &name, vcb_isfilename, NULL) == true &&
            json_get_string(line, "$.script", 0, FILENAME_LEN_MAX, &trigger_data->script, vcb_isfilename, NULL) == true &&
            json_get_int_max(line, "$.event", &event, NULL) == true &&
            json_get_object_string(line, "$.arguments", &trigger_data->arguments, vcb_isname, SCRIPT_ARGUMENTS_MAX, NULL) == true)
        {
            if (json_get_string(line, "$.partition", 1, NAME_LEN_MAX, &partition, vcb_isname, NULL) == false) {
                //fallback to default partition
                partition = sdsnew(MPD_PARTITION_DEFAULT);
            }
            if (strcmp(partition, MPD_PARTITION_ALL) == 0 ||
                check_partition_state_dir(workdir, partition) == true)
            {
                list_push(trigger_list, name, event, partition, trigger_data);
            }
            else {
                MYMPD_LOG_WARN("Skipping trigger definition for unknown partition \"%s\"", partition);
            }
        }
        else {
            mympd_api_trigger_data_free(trigger_data);
        }
        FREE_SDS(name);
        FREE_SDS(partition);
        i++;
    }
    FREE_SDS(line);
    (void) fclose(fp);
    MYMPD_LOG_INFO("Read %ld triggers(s) from disc", trigger_list->length);
    FREE_SDS(trigger_file);
    return true;
}

/**
 * Saves the trigger list to disc
 * @param trigger_list trigger list
 * @param workdir working directory
 * @return true on success, else false
 */
bool mympd_api_trigger_file_save(struct t_list *trigger_list, sds workdir) {
    MYMPD_LOG_INFO("Saving triggers to disc");
    sds filepath = sdscatfmt(sdsempty(), "%S/state/trigger_list", workdir);
    bool rc = list_write_to_disk(filepath, trigger_list, trigger_to_line_cb);
    FREE_SDS(filepath);
    return rc;
}

/**
 * Clears the trigger list
 * @param trigger_list trigger list to clear
 */
void mympd_api_trigger_list_clear(struct t_list *trigger_list) {
    list_clear_user_data(trigger_list, list_free_cb_trigger_data);
}

/**
 * Creates and initializes the t_trigger_data struct
 * @return pointer to allocated t_trigger_data struct
 */
struct t_trigger_data *trigger_data_new(void) {
    struct t_trigger_data *trigger_data = malloc_assert(sizeof(struct t_trigger_data));
    trigger_data->script = NULL;
    list_init(&trigger_data->arguments);
    return trigger_data;
}

/**
 * Frees the t_trigger_data struct
 * @param trigger_data pointer to trigger data
 */
void mympd_api_trigger_data_free(struct t_trigger_data *trigger_data) {
    FREE_SDS(trigger_data->script);
    list_clear(&trigger_data->arguments);
    FREE_PTR(trigger_data);
}

/**
 * Private functions
 */

/**
 * Callback function to free user_data of type t_trigger_data
 * @param current list node
 */
static void list_free_cb_trigger_data(struct t_list_node *current) {
    mympd_api_trigger_data_free((struct t_trigger_data *)current->user_data);
}

/**
 * Prints a trigger as a json object string
 * @param buffer already allocated sds string to append the response
 * @param current trigger node to print
 * @return pointer to buffer
 */
static sds trigger_to_line_cb(sds buffer, struct t_list_node *current) {
    struct t_trigger_data *trigger_data = (struct t_trigger_data *)current->user_data;
    buffer = sdscatlen(buffer, "{", 1);
    buffer = tojson_sds(buffer, "name", current->key, true);
    buffer = tojson_llong(buffer, "event", current->value_i, true);
    buffer = tojson_sds(buffer, "partition", current->value_p, true);
    buffer = tojson_sds(buffer, "script", trigger_data->script, true);
    buffer = sdscat(buffer, "\"arguments\":{");
    struct t_list_node *argument = trigger_data->arguments.head;
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

/**
 * Creates and pushes a request to execute a script
 * @param script script to execute
 * @param arguments script arguments
 * @param partition mpd partition
 */
void trigger_execute(sds script, struct t_list *arguments, const char *partition) {
    struct t_work_request *request = create_request(-1, 0, MYMPD_API_SCRIPT_EXECUTE, NULL, partition);
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
