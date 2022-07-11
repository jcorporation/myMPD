/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2022 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include "mympd_config_defs.h"
#include "mympd_api_timer.h"

#include "../lib/filehandler.h"
#include "../lib/jsonrpc.h"
#include "../lib/log.h"
#include "../lib/mem.h"
#include "../lib/sds_extras.h"
#include "../mpd_client/mpd_client_jukebox.h"
#include "mympd_api_timer_handlers.h"

#include <errno.h>
#include <poll.h>
#include <stdlib.h>
#include <string.h>
#include <sys/timerfd.h>
#include <unistd.h>

//private definitions
static struct t_timer_node *get_timer_from_fd(struct t_timer_list *l, int fd);
static sds print_timer_node(sds buffer, struct t_timer_node *current);

//public functions
void mympd_api_timer_timerlist_init(struct t_timer_list *l) {
    l->length = 0;
    l->active = 0;
    l->last_id = USER_TIMER_ID_START;
    l->list = NULL;
}

void mympd_api_timer_check(struct t_timer_list *l) {
    unsigned iMaxCount = 0;
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
        MYMPD_LOG_ERROR("Error polling timerfd");
        MYMPD_LOG_ERRNO(errno);
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
                MYMPD_LOG_ERROR("Could not get timer from fd");
                continue;
            }
            if (current->definition != NULL) {
                //user defined timers
                if (current->definition->enabled == false) {
                    MYMPD_LOG_DEBUG("Skipping timer with id %d, not enabled", current->timer_id);
                    continue;
                }
                time_t t = time(NULL);
                struct tm now;
                if (localtime_r(&t, &now) == NULL) {
                    MYMPD_LOG_ERROR("Localtime is NULL");
                    continue;
                }
                int wday = now.tm_wday;
                wday = wday > 0 ? wday - 1 : 6;
                if (current->definition->weekdays[wday] == false) {
                    MYMPD_LOG_DEBUG("Skipping timer with id %d, not enabled on this weekday", current->timer_id);
                    continue;
                }
            }
            //execute callback function
            MYMPD_LOG_DEBUG("Timer with id %d triggered", current->timer_id);
            if (current->callback) {
                current->callback(current->timer_id, current->definition);
            }
            //handle one shot timers
            if (current->interval == TIMER_ONE_SHOT_DISABLE &&
                current->definition != NULL)
            {
                //user defined "one shot and disable" timers
                MYMPD_LOG_DEBUG("One shot timer disabled: %d", current->timer_id);
                current->definition->enabled = false;
            }
            else if (current->interval <= TIMER_ONE_SHOT_REMOVE) {
                //"one shot and remove" timers
                MYMPD_LOG_DEBUG("One shot timer removed: %d", current->timer_id);
                mympd_api_timer_remove(l, current->timer_id);
            }
        }
    }
}

bool mympd_api_timer_replace(struct t_timer_list *l, time_t timeout, int interval, timer_handler handler,
                   int timer_id, struct t_timer_definition *definition)
{
    mympd_api_timer_remove(l, timer_id);
    return mympd_api_timer_add(l, timeout, interval, handler, timer_id, definition);
}

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
            MYMPD_LOG_ERROR("Can't create timerfd");
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
    MYMPD_LOG_DEBUG("Added timer with id %d, start time in %llds", timer_id, (long long)timeout);
    return true;
}

void mympd_api_timer_remove(struct t_timer_list *l, int timer_id) {
    struct t_timer_node *current = NULL;
    struct t_timer_node *previous = NULL;

    for (current = l->list; current != NULL; previous = current, current = current->next) {
        if (current->timer_id == timer_id) {
            MYMPD_LOG_DEBUG("Removing timer with id %d", timer_id);
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
            return;
        }
    }
}

void mympd_api_timer_toggle(struct t_timer_list *l, int timer_id) {
    struct t_timer_node *current = NULL;
    for (current = l->list; current != NULL; current = current->next) {
        if (current->timer_id == timer_id) {
            if (current->definition != NULL) {
                current->definition->enabled = current->definition->enabled == true ? false : true;
            }
            return;
        }
    }
}

void mympd_api_timer_timerlist_clear(struct t_timer_list *l) {
    struct t_timer_node *current = l->list;
    struct t_timer_node *tmp = NULL;

    while (current != NULL) {
        MYMPD_LOG_DEBUG("Removing timer with id %d", current->timer_id);
        tmp = current;
        current = current->next;
        mympd_api_timer_free_node(tmp);
    }
    mympd_api_timer_timerlist_init(l);
}

void *mympd_api_timer_free_definition(struct t_timer_definition *timer_def) {
    FREE_SDS(timer_def->name);
    FREE_SDS(timer_def->action);
    FREE_SDS(timer_def->subaction);
    FREE_SDS(timer_def->playlist);
    list_clear(&timer_def->arguments);
    FREE_PTR(timer_def);
    return NULL;
}

void *mympd_api_timer_free_node(struct t_timer_node *node) {
    if (node->fd > -1) {
        close(node->fd);
    }
    if (node->definition != NULL) {
        node->definition = mympd_api_timer_free_definition(node->definition);
    }
    FREE_PTR(node);
    return NULL;
}

struct t_timer_definition *mympd_api_timer_parse(struct t_timer_definition *timer_def, sds str, sds *error) {
    timer_def->name = NULL;
    timer_def->action = NULL;
    timer_def->subaction = NULL;
    timer_def->playlist = NULL;
    list_init(&timer_def->arguments);
    sds jukebox_mode_str = NULL;

    if (json_get_string_max(str, "$.params.name", &timer_def->name, vcb_isname, error) == true &&
        json_get_bool(str, "$.params.enabled", &timer_def->enabled, error) == true &&
        json_get_int(str, "$.params.startHour", 0, 23, &timer_def->start_hour, error) == true &&
        json_get_int(str, "$.params.startMinute", 0, 59, &timer_def->start_minute, error) == true &&
        json_get_string_max(str, "$.params.action", &timer_def->action, vcb_isalnum, error) == true &&
        json_get_string_max(str, "$.params.subaction", &timer_def->subaction, vcb_isname, error) == true &&
        json_get_uint(str, "$.params.volume", 0, 100, &timer_def->volume, error) == true &&
        json_get_string_max(str, "$.params.playlist", &timer_def->playlist, vcb_isfilename, error) == true &&
        json_get_object_string(str, "$.params.arguments", &timer_def->arguments, vcb_isname, 10, error) == true &&
        json_get_bool(str, "$.params.weekdays[0]", &timer_def->weekdays[0], error) == true &&
        json_get_bool(str, "$.params.weekdays[1]", &timer_def->weekdays[1], error) == true &&
        json_get_bool(str, "$.params.weekdays[2]", &timer_def->weekdays[2], error) == true &&
        json_get_bool(str, "$.params.weekdays[3]", &timer_def->weekdays[3], error) == true &&
        json_get_bool(str, "$.params.weekdays[4]", &timer_def->weekdays[4], error) == true &&
        json_get_bool(str, "$.params.weekdays[5]", &timer_def->weekdays[5], error) == true &&
        json_get_bool(str, "$.params.weekdays[6]", &timer_def->weekdays[6], error) == true &&
        json_get_string_max(str, "$.params.jukeboxMode", &jukebox_mode_str, vcb_isalnum, error) == true)
    {
        timer_def->jukebox_mode = mpd_client_parse_jukebox_mode(jukebox_mode_str);
        MYMPD_LOG_DEBUG("Successfully parsed timer definition");
    }
    else {
        timer_def = mympd_api_timer_free_definition(timer_def);
        MYMPD_LOG_ERROR("Error parsing timer definition");
    }
    FREE_SDS(jukebox_mode_str);
    return timer_def;
}

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

sds mympd_api_timer_list(struct t_mympd_state *mympd_state, sds buffer, sds method, long request_id) {
    buffer = jsonrpc_result_start(buffer, method, request_id);
    buffer = sdscat(buffer, "\"data\":[");
    int entities_returned = 0;
    struct t_timer_node *current = mympd_state->timer_list.list;
    while (current != NULL) {
        if (current->timer_id > 99 &&
            current->definition != NULL)
        {
            if (entities_returned++) {
                buffer = sdscatlen(buffer, ",", 1);
            }
            buffer = sdscatlen(buffer, "{", 1);
            buffer = print_timer_node(buffer, current);
            buffer = sdscatlen(buffer, "}", 1);
        }
        current = current->next;
    }

    buffer = sdscatlen(buffer, "],", 2);
    buffer = tojson_long(buffer, "returnedEntities", entities_returned, false);
    buffer = jsonrpc_result_end(buffer);
    return buffer;
}

sds mympd_api_timer_get(struct t_mympd_state *mympd_state, sds buffer, sds method, long request_id, int timer_id) {
    buffer = jsonrpc_result_start(buffer, method, request_id);
    bool found = false;
    struct t_timer_node *current = mympd_state->timer_list.list;
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

    buffer = jsonrpc_result_end(buffer);

    if (found == false) {
        buffer = jsonrpc_respond_message(buffer, method, request_id, true,
            "timer", "error", "Timer with given id not found");
    }
    return buffer;
}

bool mympd_api_timer_file_read(struct t_mympd_state *mympd_state) {
    sds timer_file = sdscatfmt(sdsempty(), "%S/state/timer_list", mympd_state->config->workdir);
    errno = 0;
    FILE *fp = fopen(timer_file, OPEN_FLAGS_READ);
    if (fp == NULL) {
        //ignore error
        MYMPD_LOG_DEBUG("Can not open file \"%s\"", timer_file);
        if (errno != ENOENT) {
            MYMPD_LOG_ERRNO(errno);
        }
        FREE_SDS(timer_file);
        return false;
    }
    int i = 0;
    sds line = sdsempty();
    sds param = sdsempty();
    while (sds_getline(&line, fp, LINE_LENGTH_MAX) == 0) {
        if (i > LIST_TIMER_MAX) {
            MYMPD_LOG_WARN("Too many timers defined");
            break;
        }
        struct t_timer_definition *timer_def = malloc_assert(sizeof(struct t_timer_definition));
        sdsclear(param);
        param = sdscatfmt(param, "{\"params\":%S}", line);
        timer_def = mympd_api_timer_parse(timer_def, param, NULL);
        int interval;
        int timerid;
        if (timer_def != NULL &&
            json_get_int(param, "$.params.interval", -1, TIMER_INTERVAL_MAX, &interval, NULL) == true &&
            json_get_int(param, "$.params.timerid", 101, 200, &timerid, NULL) == true)
        {
            if (timerid > mympd_state->timer_list.last_id) {
                mympd_state->timer_list.last_id = timerid;
            }
            time_t start = mympd_api_timer_calc_starttime(timer_def->start_hour, timer_def->start_minute, interval);
            mympd_api_timer_add(&mympd_state->timer_list, start, interval, timer_handler_select, timerid, timer_def);
        }
        else {
            MYMPD_LOG_ERROR("Invalid timer line");
            MYMPD_LOG_DEBUG("Errorneous line: %s", line);
            if (timer_def != NULL) {
                timer_def = mympd_api_timer_free_definition(timer_def);
            }
        }
        i++;
    }
    FREE_SDS(param);
    FREE_SDS(line);
    (void) fclose(fp);
    FREE_SDS(timer_file);
    MYMPD_LOG_INFO("Read %d timer(s) from disc", mympd_state->timer_list.length);
    return true;
}

bool mympd_api_timer_file_save(struct t_mympd_state *mympd_state) {
    MYMPD_LOG_INFO("Saving timers to disc");
    sds tmp_file = sdscatfmt(sdsempty(), "%S/state/timer_list.XXXXXX", mympd_state->config->workdir);
    FILE *fp = open_tmp_file(tmp_file);
    if (fp == NULL) {
        FREE_SDS(tmp_file);
        return false;
    }
    struct t_timer_node *current = mympd_state->timer_list.list;
    sds buffer = sdsempty();
    bool write_rc = true;
    while (current != NULL) {
        if (current->timer_id > 99 &&
            current->definition != NULL)
        {
            buffer = sds_replace(buffer, "{");
            buffer = print_timer_node(buffer, current);
            buffer = sdscatlen(buffer, "}\n", 2);
            if (fputs(buffer, fp) == EOF) {
                MYMPD_LOG_ERROR("Could not write timers to disc");
                write_rc = false;
                break;
            }
        }
        current = current->next;
    }
    FREE_SDS(buffer);
    sds filepath = sdscatfmt(sdsempty(), "%S/state/timer_list", mympd_state->config->workdir);
    bool rc = rename_tmp_file(fp, tmp_file, filepath, write_rc);
    FREE_SDS(tmp_file);
    FREE_SDS(filepath);
    return rc;
}

//private functions
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

static sds print_timer_node(sds buffer, struct t_timer_node *current) {
    buffer = tojson_int(buffer, "timerid", current->timer_id, true);
    buffer = tojson_sds(buffer, "name", current->definition->name, true);
    buffer = tojson_int(buffer, "interval", current->interval, true);
    buffer = tojson_bool(buffer, "enabled", current->definition->enabled, true);
    buffer = tojson_int(buffer, "startHour", current->definition->start_hour, true);
    buffer = tojson_int(buffer, "startMinute", current->definition->start_minute, true);
    buffer = tojson_sds(buffer, "action", current->definition->action, true);
    buffer = tojson_sds(buffer, "subaction", current->definition->subaction, true);
    buffer = tojson_sds(buffer, "playlist", current->definition->playlist, true);
    buffer = tojson_uint(buffer, "volume", current->definition->volume, true);
    buffer = tojson_char(buffer, "jukeboxMode", mpd_client_lookup_jukebox_mode(current->definition->jukebox_mode), true);
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
