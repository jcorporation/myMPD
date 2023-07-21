/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2023 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include "compile_time.h"
#include "src/mympd_api/timer.h"

#include "src/lib/filehandler.h"
#include "src/lib/jsonrpc.h"
#include "src/lib/log.h"
#include "src/lib/mem.h"
#include "src/lib/sds_extras.h"
#include "src/lib/state_files.h"
#include "src/mympd_api/timer_handlers.h"

#include <errno.h>
#include <poll.h>
#include <stdbool.h>
#include <string.h>
#include <sys/timerfd.h>
#include <unistd.h>

/**
 * Private definitions
 */

static void *mympd_api_timer_free_node(struct t_timer_node *node);
static struct t_timer_node *get_timer_from_fd(struct t_timer_list *l, int fd);
static sds print_timer_node(sds buffer, struct t_timer_node *current);

/**
 * Public functions
 */

/**
 * Inits the timer list
 * @param l pointer to already allocated timer list
 */
void mympd_api_timer_timerlist_init(struct t_timer_list *l) {
    l->length = 0;
    l->active = 0;
    l->last_id = USER_TIMER_ID_START;
    l->list = NULL;
}

/**
 * Checks for timers and executes the callback function
 * @param l timer list
 */
void mympd_api_timer_check(struct t_timer_list *l) {
    nfds_t iMaxCount = 0;
    struct pollfd ufds[LIST_TIMER_MAX] = {{0}};
    memset(ufds, 0, sizeof(struct pollfd) * LIST_TIMER_MAX);
    struct t_timer_node *current = l->list;
    while (current != NULL &&
           iMaxCount <= LIST_TIMER_MAX)
    {
        if (current->fd > -1) {
            ufds[iMaxCount].fd = current->fd;
            ufds[iMaxCount].events = POLLIN;
            iMaxCount++;
        }
        current = current->next;
    }
    errno = 0;
    int read_fds = poll(ufds, iMaxCount, 100);
    if (read_fds < 0) {
        MYMPD_LOG_ERROR(NULL, "Error polling timerfd");
        MYMPD_LOG_ERRNO(NULL, errno);
        return;
    }
    if (read_fds == 0) {
        //no timer triggered
        return;
    }

    for (unsigned i = 0; i < iMaxCount; i++) {
        if (ufds[i].revents & POLLIN) {
            uint64_t exp;
            ssize_t s = read(ufds[i].fd, &exp, sizeof(uint64_t));
            if (s != sizeof(uint64_t)) {
                continue;
            }
            current = get_timer_from_fd(l, ufds[i].fd);
            if (current == NULL) {
                MYMPD_LOG_ERROR(NULL, "Could not get timer from fd");
                continue;
            }
            if (current->definition != NULL) {
                //user defined timers
                if (current->definition->enabled == false) {
                    MYMPD_LOG_DEBUG(NULL, "Skipping timer with id %d, not enabled", current->timer_id);
                    continue;
                }
                time_t t = time(NULL);
                struct tm now;
                if (localtime_r(&t, &now) == NULL) {
                    MYMPD_LOG_ERROR(NULL, "Localtime is NULL");
                    continue;
                }
                int wday = now.tm_wday;
                wday = wday > 0 ? wday - 1 : 6;
                if (current->definition->weekdays[wday] == false) {
                    MYMPD_LOG_DEBUG(NULL, "Skipping timer with id %d, not enabled on this weekday", current->timer_id);
                    continue;
                }
            }
            //execute callback function
            MYMPD_LOG_DEBUG(NULL, "Timer with id %d triggered", current->timer_id);
            if (current->callback) {
                current->callback(current->timer_id, current->definition);
            }
            //handle one shot timers
            if (current->interval == TIMER_ONE_SHOT_DISABLE &&
                current->definition != NULL)
            {
                //user defined "one shot and disable" timers
                MYMPD_LOG_DEBUG(NULL, "One shot timer disabled: %d", current->timer_id);
                current->definition->enabled = false;
            }
            else if (current->interval <= TIMER_ONE_SHOT_REMOVE) {
                //"one shot and remove" timers
                MYMPD_LOG_DEBUG(NULL, "One shot timer removed: %d", current->timer_id);
                mympd_api_timer_remove(l, current->timer_id);
            }
        }
    }
}

/**
 * Saves a new or existing timer
 * @param partition_state pointer to partition state
 * @param data request data to parse
 * @param error pointer to already allocated sds string to append an error message
 * @return true on success, else false
 */
bool mympd_api_timer_save(struct t_partition_state *partition_state, sds data, sds *error) {
    if (partition_state->mympd_state->timer_list.length > LIST_TIMER_MAX) {
        *error = sdscat(*error, "Too many timers defined");
        return false;
    }
    int interval = 0;
    int timerid = 0;
    if (json_get_int(data, "$.params.interval", -1, TIMER_INTERVAL_MAX, &interval, error) == true &&
        json_get_int(data, "$.params.timerid", 0, USER_TIMER_ID_MAX, &timerid, error) == true)
    {
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
            timerid = partition_state->mympd_state->timer_list.last_id + 1;
        }
        else if (timerid < USER_TIMER_ID_MIN) {
            //existing timer
            //user defined timers must be gt 100
            MYMPD_LOG_ERROR(partition_state->name, "Timer id must be greater or equal %d, but id is: \"%d\"", USER_TIMER_ID_MAX, timerid);
            *error = sdscat(*error, "Invalid timer id");
            return false;
        }
        //parse timer definition
        struct t_timer_definition *timer_def = malloc_assert(sizeof(struct t_timer_definition));
        timer_def = mympd_api_timer_parse(timer_def, data, partition_state->name, error);
        if (timer_def == NULL) {
            *error = sdscat(*error, "Error parsing timer definition");
            //timer_def was freed by mympd_api_timer_parse
            return false;
        }
        //calculate start time and add/replace timer
        time_t start = mympd_api_timer_calc_starttime(timer_def->start_hour, timer_def->start_minute, interval);
        bool rc = mympd_api_timer_replace(&partition_state->mympd_state->timer_list, start, interval, timer_handler_select, timerid, timer_def);
        if (rc == false) {
            *error = sdscat(*error, "Saving timer failed");
            mympd_api_timer_free_definition(timer_def);
            return false;
        }
        if (new == true) {
            partition_state->mympd_state->timer_list.last_id = timerid;
        }
        return true;
    }
    return false;
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
bool mympd_api_timer_replace(struct t_timer_list *l, time_t timeout, int interval, timer_handler handler,
                   int timer_id, struct t_timer_definition *definition)
{
    //ignore return code for remove
    mympd_api_timer_remove(l, timer_id);
    return mympd_api_timer_add(l, timeout, interval, handler, timer_id, definition);
}

/**
 * Adds a timer with given timer_id
 * @param l timer list
 * @param timeout seconds when timer will run
 * @param interval reschedule timer interval
 * @param handler timer callback function
 * @param timer_id id of the timer
 * @param definition pointer to timer definition (GUI) or NULL
 * @return true on success, else false
 */
bool mympd_api_timer_add(struct t_timer_list *l, time_t timeout, int interval, timer_handler handler,
               int timer_id, struct t_timer_definition *definition)
{
    struct t_timer_node *new_node = malloc_assert(sizeof(struct t_timer_node));
    new_node->callback = handler;
    new_node->definition = definition;
    new_node->timeout = timeout;
    new_node->interval = interval;
    new_node->timer_id = timer_id;

    if (definition == NULL ||
        definition->enabled == true)
    {
        new_node->fd = timerfd_create(CLOCK_REALTIME, 0);
        if (new_node->fd == -1) {
            FREE_PTR(new_node);
            MYMPD_LOG_ERROR(NULL, "Can't create timerfd");
            return false;
        }

        struct itimerspec new_value;
        //timeout
        new_value.it_value.tv_sec = timeout;
        new_value.it_value.tv_nsec = 0;
        //interval
        //0 = oneshot and deactivate
        //-1 = oneshot and remove
        new_value.it_interval.tv_sec = interval > 0 ? interval : 0;
        new_value.it_interval.tv_nsec = 0;

        timerfd_settime(new_node->fd, 0, &new_value, NULL);
    }
    else {
        new_node->fd = -1;
    }
    //Inserting the timer node into the list
    new_node->next = l->list;
    l->list = new_node;
    l->length++;
    if (definition == NULL ||
        definition->enabled == true)
    {
        l->active++;
    }
    MYMPD_LOG_DEBUG(NULL, "Added timer with id %d, start time in %llds", timer_id, (long long)timeout);
    return true;
}

/**
 * Removes a timer with given id
 * @param l timer list
 * @param timer_id timer id to remove
 * @return true on success, else false
 */
bool mympd_api_timer_remove(struct t_timer_list *l, int timer_id) {
    struct t_timer_node *current = NULL;
    struct t_timer_node *previous = NULL;

    for (current = l->list; current != NULL; previous = current, current = current->next) {
        if (current->timer_id == timer_id) {
            MYMPD_LOG_DEBUG(NULL, "Removing timer with id %d", timer_id);
            if (previous == NULL) {
                //Fix beginning pointer
                l->list = current->next;
            }
            else {
                //Fix previous nodes next to skip over the removed node.
                previous->next = current->next;
            }
            //Deallocate the node
            if (current->definition == NULL ||
                current->definition->enabled == true)
            {
                l->active--;
            }
            mympd_api_timer_free_node(current);
            l->length--;
            return true;
        }
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
bool mympd_api_timer_toggle(struct t_timer_list *l, int timer_id, sds *error) {
    struct t_timer_node *current = NULL;
    for (current = l->list; current != NULL; current = current->next) {
        if (current->timer_id == timer_id) {
            if (current->definition != NULL) {
                current->definition->enabled = current->definition->enabled == true ? false : true;
            }
            return true;
        }
    }
    *error = sdscat(*error, "Timer with given id not found");
    return false;
}

/**
 * Clears the timer list
 * @param l timer list
 */
void mympd_api_timer_timerlist_clear(struct t_timer_list *l) {
    struct t_timer_node *current = l->list;
    struct t_timer_node *tmp = NULL;

    while (current != NULL) {
        MYMPD_LOG_DEBUG(NULL, "Removing timer with id %d", current->timer_id);
        tmp = current;
        current = current->next;
        mympd_api_timer_free_node(tmp);
    }
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
 * @param timer_def pointer to timer defintion to populate
 * @param str string to parse
 * @param partition mpd partition
 * @param error pointer to sds string to populate an error string
 * @return pointer to timer_def or NULL on error
 */
struct t_timer_definition *mympd_api_timer_parse(struct t_timer_definition *timer_def, sds str, const char *partition, sds *error) {
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
 * Calculates the next start time for a timer
 * @param start_hour start hour
 * @param start_minute start minute
 * @param interval reschedule interval
 * @return unix timestamp of next start
 */
time_t mympd_api_timer_calc_starttime(int start_hour, int start_minute, int interval) {
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
    return start - now;
}

/**
 * Gets the timer list as an jsonrpc response
 * @param timer_list timer list
 * @param buffer already allocated sds string to append the response
 * @param request_id jsonrpc request id
 * @param partition mpd partition
 * @return pointer to buffer
 */
sds mympd_api_timer_list(struct t_timer_list *timer_list, sds buffer, long request_id, const char *partition) {
    enum mympd_cmd_ids cmd_id = MYMPD_API_TIMER_LIST;
    buffer = jsonrpc_respond_start(buffer, cmd_id, request_id);
    buffer = sdscat(buffer, "\"data\":[");
    int entities_returned = 0;
    struct t_timer_node *current = timer_list->list;
    while (current != NULL) {
        if (current->timer_id >= USER_TIMER_ID_START &&
            current->definition != NULL)
        {
            if (strcmp(partition, current->definition->partition) == 0) {
                if (entities_returned++) {
                    buffer = sdscatlen(buffer, ",", 1);
                }
                buffer = sdscatlen(buffer, "{", 1);
                buffer = print_timer_node(buffer, current);
                buffer = sdscatlen(buffer, "}", 1);
            }
        }
        current = current->next;
    }

    buffer = sdscatlen(buffer, "],", 2);
    buffer = tojson_long(buffer, "returnedEntities", entities_returned, false);
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
sds mympd_api_timer_get(struct t_timer_list *timer_list, sds buffer, long request_id, int timer_id) {
    enum mympd_cmd_ids cmd_id = MYMPD_API_TIMER_GET;
    buffer = jsonrpc_respond_start(buffer, cmd_id, request_id);
    bool found = false;
    struct t_timer_node *current = timer_list->list;
    while (current != NULL) {
        if (current->timer_id == timer_id &&
            current->definition != NULL)
        {
            buffer = print_timer_node(buffer, current);
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
    while (sds_getline(&line, fp, LINE_LENGTH_MAX) >= 0) {
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
            struct t_timer_definition *timer_def = malloc_assert(sizeof(struct t_timer_definition));
            timer_def = mympd_api_timer_parse(timer_def, param, partition, NULL);
            int interval;
            int timerid;
            if (timer_def != NULL &&
                json_get_int(param, "$.params.interval", -1, TIMER_INTERVAL_MAX, &interval, NULL) == true &&
                json_get_int(param, "$.params.timerid", USER_TIMER_ID_MIN, USER_TIMER_ID_MAX, &timerid, NULL) == true)
            {
                if (timerid > timer_list->last_id) {
                    timer_list->last_id = timerid;
                }
                time_t start = mympd_api_timer_calc_starttime(timer_def->start_hour, timer_def->start_minute, interval);
                mympd_api_timer_add(timer_list, start, interval, timer_handler_select, timerid, timer_def);
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
    MYMPD_LOG_INFO(NULL, "Read %d timer(s) from disc", timer_list->length);
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
    struct t_timer_node *current = timer_list->list;
    sds buffer = sdsempty();
    bool write_rc = true;
    while (current != NULL) {
        if (current->timer_id >= USER_TIMER_ID_START &&
            current->definition != NULL)
        {
            buffer = sds_replace(buffer, "{");
            buffer = tojson_sds(buffer, "partition", current->definition->partition, true);
            buffer = print_timer_node(buffer, current);
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
 * @return NULL
 */
static void *mympd_api_timer_free_node(struct t_timer_node *node) {
    if (node->fd > -1) {
        close(node->fd);
    }
    if (node->definition != NULL) {
        mympd_api_timer_free_definition(node->definition);
    }
    FREE_PTR(node);
    return NULL;
}

/**
 * Gets the timer associated with the fd
 * @param l timer list
 * @param fd timer fd
 * @return timer for the fd
 */
static struct t_timer_node *get_timer_from_fd(struct t_timer_list *l, int fd) {
    struct t_timer_node *current = l->list;

    while (current != NULL) {
        if (current->fd == fd) {
            return current;
        }
        current = current->next;
    }
    return NULL;
}

/**
 * Prints a timer node as a json object string
 * @param buffer already allocated sds string to append the response
 * @param current timer node to print
 * @return pointer to buffer
 */
static sds print_timer_node(sds buffer, struct t_timer_node *current) {
    buffer = tojson_int(buffer, "timerid", current->timer_id, true);
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
