/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2024 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include "compile_time.h"
#include "src/mympd_api/timer.h"

#include "src/lib/datetime.h"
#include "src/lib/filehandler.h"
#include "src/lib/jsonrpc.h"
#include "src/lib/list.h"
#include "src/lib/log.h"
#include "src/lib/mem.h"
#include "src/lib/sds_extras.h"
#include "src/lib/state_files.h"
#include "src/lib/timer.h"
#include "src/mympd_api/timer_handlers.h"

#include <errno.h>
#include <stdbool.h>
#include <string.h>

/**
 * Private definitions
 */

static void mympd_api_timer_free_node(struct t_list_node *node);
static struct t_list_node *get_timer_from_fd(struct t_timer_list *l, int fd);
static sds print_timer_node(sds buffer, unsigned timer_id, struct t_timer_node *current);

/**
 * Public functions
 */

/**
 * Inits the timer list
 * @param l pointer to already allocated timer list
 */
void mympd_api_timer_timerlist_init(struct t_timer_list *l) {
    l->active = 0;
    l->last_id = USER_TIMER_ID_START;
    list_init(&l->list);
}

/**
 * Checks the timer event and executes the callback function
 * @param fd fd with POLLIN event
 * @param timer_list timer list
 */
bool mympd_api_timer_check(int fd, struct t_timer_list *timer_list) {
    struct t_list_node *current = get_timer_from_fd(timer_list, fd);
    if (current == NULL) {
        MYMPD_LOG_ERROR(NULL, "Could not get timer from fd");
        return false;
    }
    struct t_timer_node *current_timer = (struct t_timer_node *)current->user_data;
    if (current_timer->definition != NULL) {
        //user defined timers
        if (current_timer->definition->enabled == false) {
            MYMPD_LOG_DEBUG(NULL, "Skipping timer with id %" PRId64 ", not enabled", current->value_i);
            return false;
        }
        time_t t = time(NULL);
        struct tm now;
        if (localtime_r(&t, &now) == NULL) {
            MYMPD_LOG_ERROR(NULL, "Localtime is NULL");
            return false;
        }
        int wday = now.tm_wday;
        wday = wday > 0 ? wday - 1 : 6;
        if (current_timer->definition->weekdays[wday] == false) {
            MYMPD_LOG_DEBUG(NULL, "Skipping timer with id %" PRId64 ", not enabled on this weekday", current->value_i);
            return false;
        }
    }
    //execute callback function
    MYMPD_LOG_DEBUG(NULL, "Timer with id %" PRId64 " triggered", current->value_i);
    if (current_timer->callback) {
        current_timer->callback((unsigned)current->value_i, current_timer->definition);
    }
    //handle one shot timers
    if (current_timer->interval == TIMER_ONE_SHOT_DISABLE &&
        current_timer->definition != NULL)
    {
        //user defined "one shot and disable" timers
        MYMPD_LOG_DEBUG(NULL, "One shot timer disabled: %" PRId64, current->value_i);
        current_timer->definition->enabled = false;
    }
    else if (current_timer->interval <= TIMER_ONE_SHOT_REMOVE) {
        //"one shot and remove" timers
        MYMPD_LOG_DEBUG(NULL, "One shot timer removed: %" PRId64, current->value_i);
        mympd_api_timer_remove(timer_list, (unsigned)current->value_i);
    }
    return true;
}

/**
 * Saves a new or existing timer
 * @param partition_state pointer to partition state
 * @param timer_list pointer to timer list
 * @param interval timer interval
 * @param timerid the timerid
 * @param timer_def pointer to populated timer definition
 * @param error pointer to already allocated sds string to append an error message
 * @return true on success, else false
 */
bool mympd_api_timer_save(struct t_partition_state *partition_state, struct t_timer_list *timer_list, int interval, unsigned timerid,
        struct t_timer_definition *timer_def, sds *error)
{
    if (timer_list->list.length > LIST_TIMER_MAX) {
        *error = sdscat(*error, "Too many timers defined");
        return false;
    }

    bool new = timerid == 0
        ? true
        : false;
    if (interval > 0 &&
        interval < TIMER_INTERVAL_MIN)
    {
        //interval must be gt 5 seconds
        MYMPD_LOG_ERROR(partition_state->name, "Timer interval must be greater or equal %d, but id is: \"%d\"", TIMER_INTERVAL_MIN, interval);
        *error = sdscat(*error, "Invalid timer interval");
        return false;
    }
    if (new == true) {
        timerid = timer_list->last_id + 1;
    }
    else if (timerid < USER_TIMER_ID_MIN) {
        //existing timer
        //user defined timers must be gt 100
        MYMPD_LOG_ERROR(partition_state->name, "Timer id must be greater or equal %d, but id is: \"%u\"", USER_TIMER_ID_MAX, timerid);
        *error = sdscat(*error, "Invalid timer id");
        return false;
    }
    //calculate start time and add/replace timer
    int start_in = mympd_api_timer_calc_starttime(timer_def->start_hour, timer_def->start_minute, interval);
    bool rc = mympd_api_timer_replace(timer_list, start_in, interval, timer_handler_select, timerid, timer_def);
    if (rc == false) {
        *error = sdscat(*error, "Saving timer failed");
        return false;
    }
    if (new == true) {
        timer_list->last_id = timerid;
    }
    return true;
}

/**
 * Replaces a timer with given timer_id
 * @param l timer list
 * @param timeout seconds when timer will run
 * @param interval reschedule timer interval
 * @param handler timer callback function
 * @param timer_id id of the timer
 * @param definition pointer to timer definition (GUI) or NULL
 * @return true on success, else false
 */
bool mympd_api_timer_replace(struct t_timer_list *l, int timeout, int interval, timer_handler handler,
        unsigned timer_id, struct t_timer_definition *definition)
{
    //ignore return code for remove
    mympd_api_timer_remove(l, timer_id);
    return mympd_api_timer_add(l, timeout, interval, handler, timer_id, definition);
}

/**
 * Adds a timer with given timer_id
 * @param l timer list
 * @param timeout seconds when timer will run (offset from now)
 * @param interval reschedule timer interval
 * @param handler timer callback function
 * @param timer_id id of the timer
 * @param definition pointer to timer definition (GUI) or NULL
 * @return true on success, else false
 */
bool mympd_api_timer_add(struct t_timer_list *l, int timeout, int interval, timer_handler handler,
        unsigned timer_id, struct t_timer_definition *definition)
{
    struct t_timer_node *new_node = malloc_assert(sizeof(struct t_timer_node));
    new_node->callback = handler;
    new_node->definition = definition;
    new_node->timeout = timeout;
    new_node->interval = interval;

    if (definition == NULL ||           // internal timers
        definition->enabled == true)    // user defined timers
    {
        // Interval:
        //  0 = oneshot and deactivate
        // -1 = oneshot and remove
        new_node->fd = mympd_timer_create(CLOCK_REALTIME, timeout, (interval > 0 ? interval : 0));
        if (new_node->fd == -1) {
            FREE_PTR(new_node);
            return false;
        }
    }
    else {
        new_node->fd = -1;
    }
    list_push(&l->list, "", timer_id, NULL, new_node);
    if (definition == NULL ||
        definition->enabled == true)
    {
        l->active++;
    }
    #ifdef MYMPD_DEBUG
        char fmt_time[32];
        readable_time(fmt_time, timeout);
        MYMPD_LOG_DEBUG(NULL, "Added timer with id %u, start time %s", timer_id, fmt_time);
    #endif
    return true;
}

/**
 * Removes a timer with given id
 * @param l timer list
 * @param timer_id timer id to remove
 * @return true on success, else false
 */
bool mympd_api_timer_remove(struct t_timer_list *l, unsigned timer_id) {
    struct t_list_node *current = l->list.head;
    unsigned idx = 0;
    while (current != NULL) {
        if (current->value_i == timer_id) {
            break;
        }
        idx++;
        current = current->next;
    }
    if (current != NULL) {
        struct t_timer_node *timer_node = (struct t_timer_node *)current->user_data;
        if (timer_node->definition == NULL ||
            timer_node->definition->enabled == true)
        {
            l->active--;
        }
        list_remove_node_user_data(&l->list, idx, mympd_api_timer_free_node);
        return true;
    }
    return false;
}

/**
 * Toggles the enabled state of a timer
 * @param l timer list
 * @param timer_id timer id to toggle
 * @param error pointer to already allocated sds string to append an error message
 * @return true on success, else false
 */
bool mympd_api_timer_toggle(struct t_timer_list *l, unsigned timer_id, sds *error) {
    struct t_list_node *current = l->list.head;
    while (current != NULL) {
        if (current->value_i == timer_id) {
            struct t_timer_node *current_timer = (struct t_timer_node *)current->user_data;
            if (current_timer->definition != NULL) {
                current_timer->definition->enabled = current_timer->definition->enabled == true
                    ? false
                    : true;
            }
            return true;
        }
        current = current->next;
    }
    *error = sdscat(*error, "Timer with given id not found");
    return false;
}

/**
 * Clears the timer list
 * @param l timer list
 */
void mympd_api_timer_timerlist_clear(struct t_timer_list *l) {
    list_clear_user_data(&l->list, mympd_api_timer_free_node);
    mympd_api_timer_timerlist_init(l);
}

/**
 * Frees a timer definition
 * @param timer_def pointer to timer definition
 * @return NULL
 */
void *mympd_api_timer_free_definition(struct t_timer_definition *timer_def) {
    FREE_SDS(timer_def->name);
    FREE_SDS(timer_def->partition);
    FREE_SDS(timer_def->action);
    FREE_SDS(timer_def->subaction);
    FREE_SDS(timer_def->playlist);
    FREE_SDS(timer_def->preset);
    list_clear(&timer_def->arguments);
    FREE_PTR(timer_def);
    return NULL;
}

/**
 * Parses a json object string to a timer definition.
 * Frees the timer and sets the pointer to NULL if there is a parsing error.
 * @param str string to parse
 * @param partition mpd partition
 * @param error pointer to sds string to populate an error string
 * @return pointer to timer_def or NULL on error
 */
struct t_timer_definition *mympd_api_timer_parse(sds str, const char *partition, struct t_jsonrpc_parse_error *error) {
    struct t_timer_definition *timer_def = malloc_assert(sizeof(struct t_timer_definition));
    timer_def->name = NULL;
    timer_def->partition = NULL;
    timer_def->action = NULL;
    timer_def->subaction = NULL;
    timer_def->playlist = NULL;
    timer_def->preset = NULL;
    list_init(&timer_def->arguments);

    if (json_get_string_max(str, "$.params.name", &timer_def->name, vcb_isname, error) == true &&
        json_get_bool(str, "$.params.enabled", &timer_def->enabled, error) == true &&
        json_get_int(str, "$.params.startHour", 0, 23, &timer_def->start_hour, error) == true &&
        json_get_int(str, "$.params.startMinute", 0, 59, &timer_def->start_minute, error) == true &&
        json_get_string_max(str, "$.params.action", &timer_def->action, vcb_isalnum, error) == true &&
        json_get_string_max(str, "$.params.subaction", &timer_def->subaction, vcb_isname, error) == true &&
        json_get_string_max(str, "$.params.preset", &timer_def->preset, vcb_isname, error) == true &&
        json_get_uint(str, "$.params.volume", VOLUME_MIN, VOLUME_MAX, &timer_def->volume, error) == true &&
        json_get_string_max(str, "$.params.playlist", &timer_def->playlist, vcb_isfilename, error) == true &&
        json_get_object_string(str, "$.params.arguments", &timer_def->arguments, vcb_isname, SCRIPT_ARGUMENTS_MAX, error) == true &&
        json_get_bool(str, "$.params.weekdays[0]", &timer_def->weekdays[0], error) == true &&
        json_get_bool(str, "$.params.weekdays[1]", &timer_def->weekdays[1], error) == true &&
        json_get_bool(str, "$.params.weekdays[2]", &timer_def->weekdays[2], error) == true &&
        json_get_bool(str, "$.params.weekdays[3]", &timer_def->weekdays[3], error) == true &&
        json_get_bool(str, "$.params.weekdays[4]", &timer_def->weekdays[4], error) == true &&
        json_get_bool(str, "$.params.weekdays[5]", &timer_def->weekdays[5], error) == true &&
        json_get_bool(str, "$.params.weekdays[6]", &timer_def->weekdays[6], error) == true)
    {
        timer_def->partition = sdsnew(partition);
        MYMPD_LOG_DEBUG(NULL, "Successfully parsed timer definition");
    }
    else {
        timer_def = mympd_api_timer_free_definition(timer_def);
        MYMPD_LOG_ERROR(NULL, "Error parsing timer definition");
    }
    return timer_def;
}

/**
 * Calculates the offset from now for next start time for a timer
 * @param start_hour start hour
 * @param start_minute start minute
 * @param interval reschedule interval
 * @return unix timestamp of next start
 */
int mympd_api_timer_calc_starttime(int start_hour, int start_minute, int interval) {
    time_t now = time(NULL);
    struct tm tms;
    localtime_r(&now, &tms);
    tms.tm_hour = start_hour;
    tms.tm_min = start_minute;
    tms.tm_sec = 0;
    time_t start = mktime(&tms);

    if (interval <= 0) {
        interval = 86400;
    }

    while (start < now) {
        start += interval;
    }
    return (int)(start - now);
}

/**
 * Gets the timer list as an jsonrpc response
 * @param timer_list timer list
 * @param buffer already allocated sds string to append the response
 * @param request_id jsonrpc request id
 * @param partition mpd partition
 * @return pointer to buffer
 */
sds mympd_api_timer_list(struct t_timer_list *timer_list, sds buffer, unsigned request_id, const char *partition) {
    enum mympd_cmd_ids cmd_id = MYMPD_API_TIMER_LIST;
    buffer = jsonrpc_respond_start(buffer, cmd_id, request_id);
    buffer = sdscat(buffer, "\"data\":[");
    unsigned entities_returned = 0;
    struct t_list_node *current = timer_list->list.head;
    while (current != NULL) {
        struct t_timer_node *current_timer = (struct t_timer_node *)current->user_data;
        if (current->value_i >= USER_TIMER_ID_START &&
            current_timer->definition != NULL)
        {
            if (strcmp(partition, current_timer->definition->partition) == 0) {
                if (entities_returned++) {
                    buffer = sdscatlen(buffer, ",", 1);
                }
                buffer = sdscatlen(buffer, "{", 1);
                buffer = print_timer_node(buffer, (unsigned)current->value_i, current_timer);
                buffer = sdscatlen(buffer, "}", 1);
            }
        }
        current = current->next;
    }
    buffer = sdscatlen(buffer, "],", 2);
    buffer = tojson_uint(buffer, "returnedEntities", entities_returned, false);
    buffer = jsonrpc_end(buffer);
    return buffer;
}

/**
 * Gets the timer with the given id as an jsonrpc response
 * @param timer_list timer list
 * @param buffer already allocated sds string to append the response
 * @param request_id jsonrpc request id
 * @param timer_id timer id
 * @return pointer to buffer
 */
sds mympd_api_timer_get(struct t_timer_list *timer_list, sds buffer, unsigned request_id, unsigned timer_id) {
    enum mympd_cmd_ids cmd_id = MYMPD_API_TIMER_GET;
    buffer = jsonrpc_respond_start(buffer, cmd_id, request_id);
    bool found = false;
    struct t_list_node *current = timer_list->list.head;
    while (current != NULL) {
        struct t_timer_node *current_timer = (struct t_timer_node *)current->user_data;
        if (current->value_i == timer_id &&
            current_timer->definition != NULL)
        {
            buffer = print_timer_node(buffer, (unsigned)current->value_i, current_timer);
            found = true;
            break;
        }
        current = current->next;
    }
    buffer = jsonrpc_end(buffer);

    if (found == false) {
        buffer = jsonrpc_respond_message(buffer, cmd_id, request_id,
            JSONRPC_FACILITY_TIMER, JSONRPC_SEVERITY_ERROR, "Timer with given id not found");
    }
    return buffer;
}

/**
 * Reads the timer file and populates the timer list
 * @param timer_list timer list
 * @param workdir working directory
 * @return true on success, else false
 */
bool mympd_api_timer_file_read(struct t_timer_list *timer_list, sds workdir) {
    sds timer_file = sdscatfmt(sdsempty(), "%S/%s/%s", workdir, DIR_WORK_STATE, FILENAME_TIMER);
    errno = 0;
    FILE *fp = fopen(timer_file, OPEN_FLAGS_READ);
    if (fp == NULL) {
        //ignore error
        MYMPD_LOG_DEBUG(NULL, "Can not open file \"%s\"", timer_file);
        if (errno != ENOENT) {
            MYMPD_LOG_ERRNO(NULL, errno);
        }
        FREE_SDS(timer_file);
        return false;
    }
    int i = 0;
    sds line = sdsempty();
    sds param = sdsempty();
    int nread = 0;
    while ((line = sds_getline(line, fp, LINE_LENGTH_MAX, &nread)) && nread >= 0) {
        if (i > LIST_TIMER_MAX) {
            MYMPD_LOG_WARN(NULL, "Too many timers defined");
            break;
        }
        sdsclear(param);
        param = sdscatfmt(param, "{\"params\":%S}", line);
        sds partition = NULL;
        if (json_get_string_max(param, "$.params.partition", &partition, vcb_isname, NULL) == false) {
            //fallback to default partition
            partition = sdsnew(MPD_PARTITION_DEFAULT);
        }
        if (check_partition_state_dir(workdir, partition) == true) {
            struct t_timer_definition *timer_def = mympd_api_timer_parse(param, partition, NULL);
            int interval;
            unsigned timerid;
            if (timer_def != NULL &&
                json_get_int(param, "$.params.interval", -1, TIMER_INTERVAL_MAX, &interval, NULL) == true &&
                json_get_uint(param, "$.params.timerid", USER_TIMER_ID_MIN, USER_TIMER_ID_MAX, &timerid, NULL) == true)
            {
                if (timerid > timer_list->last_id) {
                    timer_list->last_id = timerid;
                }
                int start_in = mympd_api_timer_calc_starttime(timer_def->start_hour, timer_def->start_minute, interval);
                mympd_api_timer_add(timer_list, start_in, interval, timer_handler_select, timerid, timer_def);
            }
            else {
                MYMPD_LOG_ERROR(NULL, "Invalid timer line");
                MYMPD_LOG_DEBUG(NULL, "Errorneous line: %s", line);
                if (timer_def != NULL) {
                    mympd_api_timer_free_definition(timer_def);
                }
            }
            i++;
        }
        else {
            MYMPD_LOG_WARN(NULL, "Skipping timer definition for unknown partition \"%s\"", partition);
        }
        FREE_SDS(partition);
    }
    FREE_SDS(param);
    FREE_SDS(line);
    (void) fclose(fp);
    FREE_SDS(timer_file);
    MYMPD_LOG_INFO(NULL, "Read %u timer(s) from disc", timer_list->list.length);
    return true;
}

/**
 * Saves the timer list to the timer file on disc
 * @param timer_list timer list
 * @param workdir working directory
 * @return true on success, else false
 */
bool mympd_api_timer_file_save(struct t_timer_list *timer_list, sds workdir) {
    MYMPD_LOG_INFO(NULL, "Saving timers to disc");
    sds tmp_file = sdscatfmt(sdsempty(), "%S/%s/%s.XXXXXX", workdir, DIR_WORK_STATE, FILENAME_TIMER);
    FILE *fp = open_tmp_file(tmp_file);
    if (fp == NULL) {
        FREE_SDS(tmp_file);
        return false;
    }
    struct t_list_node *current = timer_list->list.head;
    sds buffer = sdsempty();
    bool write_rc = true;
    while (current != NULL) {
        struct t_timer_node *current_timer = (struct t_timer_node *)current->user_data;
        if (current->value_i >= USER_TIMER_ID_START &&
            current_timer->definition != NULL)
        {
            buffer = sds_replace(buffer, "{");
            buffer = tojson_sds(buffer, "partition", current_timer->definition->partition, true);
            buffer = print_timer_node(buffer, (unsigned)current->value_i, current_timer);
            buffer = sdscatlen(buffer, "}\n", 2);
            if (fputs(buffer, fp) == EOF) {
                MYMPD_LOG_ERROR(NULL, "Could not write timers to disc");
                write_rc = false;
                break;
            }
        }
        current = current->next;
    }
    FREE_SDS(buffer);
    bool rc = rename_tmp_file(fp, tmp_file, write_rc);
    FREE_SDS(tmp_file);
    return rc;
}

/**
 * Private functions
 */

/**
 * Frees a timer node
 * @param node timer node to free
 */
static void mympd_api_timer_free_node(struct t_list_node *node) {
    struct t_timer_node *timer = (struct t_timer_node *)node->user_data;
    mympd_timer_close(timer->fd);
    if (timer->definition != NULL) {
        mympd_api_timer_free_definition(timer->definition);
    }
    FREE_PTR(timer);
}

/**
 * Gets the timer associated with the fd
 * @param l timer list
 * @param fd timer fd
 * @return timer for the fd
 */
static struct t_list_node *get_timer_from_fd(struct t_timer_list *l, int fd) {
    struct t_list_node *current = l->list.head;
    while (current != NULL) {
        struct t_timer_node *current_timer = (struct t_timer_node *)current->user_data;
        if (current_timer->fd == fd) {
            return current;
        }
        current = current->next;
    }
    return NULL;
}

/**
 * Prints a timer node as a json object string
 * @param buffer already allocated sds string to append the response
 * @param timer_id the timer id
 * @param current timer node to print
 * @return pointer to buffer
 */
static sds print_timer_node(sds buffer, unsigned timer_id, struct t_timer_node *current) {
    buffer = tojson_uint(buffer, "timerid", timer_id, true);
    buffer = tojson_sds(buffer, "name", current->definition->name, true);
    buffer = tojson_int(buffer, "interval", current->interval, true);
    buffer = tojson_bool(buffer, "enabled", current->definition->enabled, true);
    buffer = tojson_int(buffer, "startHour", current->definition->start_hour, true);
    buffer = tojson_int(buffer, "startMinute", current->definition->start_minute, true);
    buffer = tojson_sds(buffer, "action", current->definition->action, true);
    buffer = tojson_sds(buffer, "subaction", current->definition->subaction, true);
    buffer = tojson_uint(buffer, "volume", current->definition->volume, true);
    buffer = tojson_sds(buffer, "playlist", current->definition->playlist, true);
    buffer = tojson_char(buffer, "preset", current->definition->preset, true);
    buffer = sdscat(buffer, "\"weekdays\":[");
    for (int i = 0; i < 7; i++) {
        if (i > 0) {
            buffer = sdscatlen(buffer, ",", 1);
        }
        buffer = sds_catbool(buffer, current->definition->weekdays[i]);
    }
    buffer = sdscat(buffer, "],\"arguments\": {");
    struct t_list_node *argument = current->definition->arguments.head;
    int i = 0;
    while (argument != NULL) {
        if (i++) {
            buffer = sdscatlen(buffer, ",", 1);
        }
        buffer = tojson_sds(buffer, argument->key, argument->value_p, false);
        argument = argument->next;
    }
    buffer = sdscatlen(buffer, "}", 1);
    return buffer;
}
