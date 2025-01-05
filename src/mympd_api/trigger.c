/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2025 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

/*! \file
 * \brief myMPD trigger API
 */

#include "compile_time.h"
#include "dist/sds/sds.h"
#include "src/lib/sticker.h"
#include "src/mympd_api/trigger.h"

#include "src/lib/api.h"
#include "src/lib/filehandler.h"
#include "src/lib/jsonrpc.h"
#include "src/lib/log.h"
#include "src/lib/mem.h"
#include "src/lib/sds_extras.h"
#include "src/lib/state_files.h"

#include "src/scripts/events.h"

#include <errno.h>
#include <string.h>

/**
 * Private definitions
 */

static void list_free_cb_trigger_data(struct t_list_node *current);
static sds trigger_to_line_cb(sds buffer, struct t_list_node *current, bool newline);
void trigger_execute(sds script, enum script_start_events script_event, struct t_list *arguments, const char *partition,
        unsigned long conn_id, unsigned request_id);

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
    "mpd_sticker",           // triggered through stickerdb connection
    "mpd_subscription",
    "mpd_message",
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
    "mympd_skipped",
    "mympd_lyrics",
    "mympd_albumart",
    "mympd_tagart",
    "mympd_jukebox",
    "mympd_smartpls",
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
const char *mympd_api_event_name(int event) {
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
        buffer = tojson_int(buffer, mympd_event_names[i], (-1 - i), true);
    }

    for (int i = 0; trigger_event_names[i] != NULL; ++i) {
        if (i > 0) {
            buffer = sdscatlen(buffer, ",", 1);
        }
        buffer = tojson_int(buffer, trigger_event_names[i], (1 << i), false);
    }
    return buffer;
}

/**
 * Executes all scripts associated with the trigger
 * @param trigger_list trigger list
 * @param event trigger to execute scripts for
 * @param partition mpd partition
 * @param arguments list of script arguments
 * @return number of executed triggers
 */
int mympd_api_trigger_execute(struct t_list *trigger_list, enum trigger_events event,
        const char *partition, struct t_list *arguments)
{
    MYMPD_LOG_DEBUG(partition, "Trigger event: %s (%d)", mympd_api_event_name(event), event);
    struct t_list_node *current = trigger_list->head;
    int n = 0;
    while (current != NULL) {
        if (current->value_i == event &&
                (strcmp(partition, current->value_p) == 0 ||
                 strcmp(current->value_p, MPD_PARTITION_ALL) == 0)
           )
        {
            struct t_trigger_data *trigger_data = (struct t_trigger_data *)current->user_data;
            MYMPD_LOG_NOTICE(partition, "Executing script \"%s\" for trigger \"%s\" (%d)",
                trigger_data->script, mympd_api_event_name(event), event);
            struct t_list *script_arguments = list_dup(&trigger_data->arguments);
            if (arguments != NULL) {
                list_append(script_arguments, arguments);
            }
            trigger_execute(trigger_data->script, SCRIPT_START_TRIGGER, script_arguments, partition, 0, 0);
            n++;
        }
        current = current->next;
    }
    return n;
}

/**
 * Executes triggers for http output
 * @param trigger_list trigger list
 * @param event trigger to execute scripts for
 * @param partition mpd partition
 * @param conn_id mongoose connection id
 * @param request_id jsonprc id
 * @param arguments list of script arguments
 * @return number of executed triggers
 */
int mympd_api_trigger_execute_http(struct t_list *trigger_list, enum trigger_events event,
        const char *partition, unsigned long conn_id, unsigned request_id,
        struct t_list *arguments)
{
    MYMPD_LOG_DEBUG(partition, "HTTP trigger event: %s (%d)", mympd_api_event_name(event), event);
    int n = 0;
    struct t_list_node *current = trigger_list->head;
    while (current != NULL) {
        if (current->value_i == event &&
                (strcmp(partition, current->value_p) == 0 ||
                 strcmp(current->value_p, MPD_PARTITION_ALL) == 0)
           )
        {
            struct t_trigger_data *trigger_data = (struct t_trigger_data *)current->user_data;
            MYMPD_LOG_NOTICE(partition, "Executing script \"%s\" for trigger \"%s\" (%d)",
                trigger_data->script, mympd_api_event_name(event), event);
            struct t_list *script_arguments = list_new();
            if (arguments != NULL) {
                list_append(script_arguments, arguments);
            }
            trigger_execute(trigger_data->script, SCRIPT_START_HTTP, script_arguments, partition, conn_id, request_id);
            n++;
        }
        current = current->next;
    }
    return n;
}

/**
 * Executes the feedback trigger
 * @param trigger_list trigger list
 * @param uri feedback uri
 * @param type feedback type
 * @param value the feedback
 * @param partition mpd partition
 * @return number of executed triggers
 */
int mympd_api_trigger_execute_feedback(struct t_list *trigger_list, sds uri, enum mympd_feedback_type type,
        int value, const char *partition)
{
    MYMPD_LOG_DEBUG(partition, "Trigger event: mympd_feedback (-6) for \"%s\", type %d, value %d", uri, type, value);
    //trigger mympd_feedback executes scripts with uri and vote arguments
    sds vote_str = sdsfromlonglong((long long)value);
    const char *type_str = type == FEEDBACK_LIKE
            ? "like"
            : "rating";

    struct t_list arguments;
    list_init(&arguments);
    list_push(&arguments, "uri", 0, uri, NULL);
    list_push(&arguments, "vote", 0, vote_str, NULL);
    list_push(&arguments, "type", 0, type_str, NULL);
    int n = mympd_api_trigger_execute(trigger_list, TRIGGER_MYMPD_FEEDBACK, partition, &arguments);
    list_clear(&arguments);
    FREE_SDS(vote_str);
    return n;
}

/**
 * Saves a trigger
 * @param trigger_list trigger list
 * @param name trigger name
 * @param trigger_id existing trigger id to replace or -1
 * @param event trigger event
 * @param partition mpd partition
 * @param trigger_data the trigger data (script name and arguments)
 * @param error already allocated sds string to append the error message
 * @return true on success, else false
 */
bool mympd_api_trigger_save(struct t_list *trigger_list, sds name, int trigger_id, int event, sds partition,
        struct t_trigger_data *trigger_data, sds *error)
{
    // delete old trigger, ignore error
    if (trigger_id >= 0 &&
        mympd_api_trigger_delete(trigger_list, (unsigned)trigger_id, error) == false)
    {
        return false;
    }

    bool rc = list_push(trigger_list, name, event, partition, trigger_data);
    if (rc == false) {
        *error = sdscat(*error, "Could not save trigger");
    }
    return rc;
}

/**
 * Deletes a trigger
 * @param trigger_list trigger list
 * @param idx index of trigger node to remove
 * @param error already allocated sds string to append the error message
 * @return true on success, else false
 */
bool mympd_api_trigger_delete(struct t_list *trigger_list, unsigned idx, sds *error) {
    struct t_list_node *to_remove = list_node_extract(trigger_list, idx);
    if (to_remove != NULL) {
        list_node_free_user_data(to_remove, list_free_cb_trigger_data);
        return true;
    }
    MYMPD_LOG_ERROR(NULL, "Trigger with id %u not found", idx);
    *error = sdscat(*error, "Could not delete trigger");
    return false;
}

/**
 * Prints the trigger list as jsonrpc response
 * @param trigger_list trigger list
 * @param buffer already allocated sds string to append the response
 * @param request_id jsonrpc request id
 * @param partition mpd partition
 * @return pointer to buffer
 */
sds mympd_api_trigger_list(struct t_list *trigger_list, sds buffer, unsigned request_id, const char *partition) {
    enum mympd_cmd_ids cmd_id = MYMPD_API_TRIGGER_GET;
    buffer = jsonrpc_respond_start(buffer, cmd_id, request_id);
    buffer = sdscat(buffer, "\"data\":[");
    unsigned entities_returned = 0;
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
            buffer = tojson_int64(buffer, "event", current->value_i, true);
            buffer = tojson_char(buffer, "eventName", mympd_api_event_name((int)current->value_i), true);
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
    buffer = tojson_uint(buffer, "returnedEntities", entities_returned, false);
    buffer = jsonrpc_end(buffer);
    return buffer;
}

/**
 * Prints the trigger with given id as jsonrpc response
 * @param trigger_list trigger list
 * @param buffer already allocated sds string to append the response
 * @param request_id jsonrpc request id
 * @param trigger_id trigger id to print
 * @return pointer to buffer
 */
sds mympd_api_trigger_get(struct t_list *trigger_list, sds buffer, unsigned request_id, unsigned trigger_id) {
    enum mympd_cmd_ids cmd_id = MYMPD_API_TRIGGER_GET;
    struct t_list_node *current = list_node_at(trigger_list, trigger_id);
    if (current != NULL) {
        struct t_trigger_data *trigger_data = (struct t_trigger_data *)current->user_data;
        buffer = jsonrpc_respond_start(buffer, cmd_id, request_id);
        buffer = tojson_uint(buffer, "id", trigger_id, true);
        buffer = tojson_sds(buffer, "name", current->key, true);
        buffer = tojson_int64(buffer, "event", current->value_i, true);
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
 * Reads the trigger file from disc and populates the trigger list
 * @param trigger_list trigger list
 * @param workdir working directory
 * @return true on success, else false
 */
bool mympd_api_trigger_file_read(struct t_list *trigger_list, sds workdir) {
    sds trigger_file = sdscatfmt(sdsempty(), "%S/%s/%s", workdir, DIR_WORK_STATE, FILENAME_TRIGGER);
    errno = 0;
    FILE *fp = fopen(trigger_file, OPEN_FLAGS_READ);
    if (fp == NULL) {
        MYMPD_LOG_DEBUG(NULL, "Can not open file \"%s\"", trigger_file);
        if (errno != ENOENT) {
            MYMPD_LOG_ERRNO(NULL, errno);
        }
        FREE_SDS(trigger_file);
        return false;
    }
    int i = 0;
    sds line = sdsempty();
    int nread = 0;
    while ((line = sds_getline(line, fp, LINE_LENGTH_MAX, &nread)) && nread >= 0) {
        if (i > LIST_TRIGGER_MAX) {
            MYMPD_LOG_WARN(NULL, "Too many triggers defined");
            break;
        }
        if (validate_json_object(line) == false) {
            MYMPD_LOG_ERROR(NULL, "Invalid line");
            break;
        }
        sds name = NULL;
        sds partition = NULL;
        int event;
        struct t_trigger_data *trigger_data = trigger_data_new();
        if (json_get_string(line, "$.name", 1, FILENAME_LEN_MAX, &name, vcb_isfilename, NULL) == true &&
            json_get_string(line, "$.script", 0, FILENAME_LEN_MAX, &trigger_data->script, vcb_isfilename, NULL) == true &&
            json_get_int_max(line, "$.event", &event, NULL) == true &&
            json_get_object_string(line, "$.arguments", &trigger_data->arguments, vcb_isalnum, vcb_isname, SCRIPT_ARGUMENTS_MAX, NULL) == true)
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
                MYMPD_LOG_WARN(NULL, "Skipping trigger definition for unknown partition \"%s\"", partition);
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
    MYMPD_LOG_INFO(NULL, "Read %u triggers(s) from disc", trigger_list->length);
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
    MYMPD_LOG_INFO(NULL, "Saving %u triggers to disc", trigger_list->length);
    sds filepath = sdscatfmt(sdsempty(), "%S/%s/%s", workdir, DIR_WORK_STATE, FILENAME_TRIGGER);
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
 * @param newline append a newline char
 * @return pointer to buffer
 */
static sds trigger_to_line_cb(sds buffer, struct t_list_node *current, bool newline) {
    struct t_trigger_data *trigger_data = (struct t_trigger_data *)current->user_data;
    buffer = sdscatlen(buffer, "{", 1);
    buffer = tojson_sds(buffer, "name", current->key, true);
    buffer = tojson_int64(buffer, "event", current->value_i, true);
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
    buffer = sdscatlen(buffer, "}}", 2);
    if (newline == true) {
        buffer = sdscatlen(buffer, "\n", 1);
    }
    return buffer;
}

/**
 * Creates and pushes a request to execute a script
 * @param script script to execute
 * @param script_event script start event
 * @param arguments script arguments
 * @param partition mpd partition
 * @param conn_id mongoose connection id
 * @param request_id jsonprc id
 */
void trigger_execute(sds script, enum script_start_events script_event, struct t_list *arguments, const char *partition,
        unsigned long conn_id, unsigned request_id)
{
    #ifdef MYMPD_ENABLE_LUA
        struct t_work_request *request = create_request(REQUEST_TYPE_DISCARD, conn_id, request_id, INTERNAL_API_SCRIPT_EXECUTE, "", partition);
        struct t_script_execute_data *extra = script_execute_data_new(script, script_event);
        extra->arguments = arguments;
        request->extra = extra;
        push_request(request, 0);
    #else
        (void) script;
        (void) script_event;
        (void) arguments;
        (void) partition;
        (void) conn_id;
        (void) request_id;
    #endif
}
