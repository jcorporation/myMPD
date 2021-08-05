/*
 SPDX-License-Identifier: GPL-2.0-or-later
 myMPD (c) 2018-2021 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include "mympd_config_defs.h"
#include "mympd_api_timer.h"

#include "../../dist/src/frozen/frozen.h"
#include "../log.h"
#include "../sds_extras.h"
#include "../utility.h"
#include "mympd_api_timer_handlers.h"

#include <assert.h>
#include <errno.h>
#include <poll.h>
#include <stdlib.h>
#include <string.h>
#include <sys/timerfd.h>
#include <unistd.h>

//private definitions
#define MAX_TIMER_COUNT 100

static struct t_timer_node *get_timer_from_fd(struct t_timer_list *l, int fd);

//public functions
void init_timerlist(struct t_timer_list *l) {
    l->length = 0;
    l->active = 0;
    l->last_id = 100;
    l->list = NULL;
}

void check_timer(struct t_timer_list *l) {
    int iMaxCount = 0;
    struct t_timer_node *current = l->list;
    uint64_t exp;

    struct pollfd ufds[MAX_TIMER_COUNT] = {{0}};
    memset(ufds, 0, sizeof(struct pollfd) * MAX_TIMER_COUNT);
    while (current != NULL && iMaxCount <= 100) {
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
        return;
    }

    for (int i = 0; i < iMaxCount; i++) {
        if (ufds[i].revents & POLLIN) {
            int s = read(ufds[i].fd, &exp, sizeof(uint64_t));
            if (s != sizeof(uint64_t)) {
                continue;
            }
            current = get_timer_from_fd(l, ufds[i].fd);
            if (current) {
                if (current->definition != NULL) {
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
                MYMPD_LOG_DEBUG("Timer with id %d triggered", current->timer_id);
                if (current->callback) {
                    current->callback(current->definition, current->user_data);
                }
                if (current->interval == 0 && current->definition != NULL) {
                    //one shot and deactivate
                    //only for timers from ui
                    MYMPD_LOG_DEBUG("One shot timer disabled: %d", current->timer_id);
                    current->definition->enabled = false;
                }
                else if (current->interval <= 0) {
                    //one shot and remove
                    //not ui timers are also removed
                    MYMPD_LOG_DEBUG("One shot timer removed: %d", current->timer_id);
                    remove_timer(l, current->timer_id);
                }
            }
        }
    }
}

bool replace_timer(struct t_timer_list *l, unsigned int timeout, int interval, time_handler handler, 
                   int timer_id, struct t_timer_definition *definition, void *user_data)
{
    remove_timer(l, timer_id);
    return add_timer(l, timeout, interval, handler, timer_id, definition, user_data);
}

bool add_timer(struct t_timer_list *l, unsigned int timeout, int interval, time_handler handler, 
               int timer_id, struct t_timer_definition *definition, void *user_data) 
{

    if (l->length == 100) {
        MYMPD_LOG_ERROR("Maximum number of timers (100) reached");
        return false;
    }

    struct t_timer_node *new_node = (struct t_timer_node *)malloc(sizeof(struct t_timer_node));
    assert(new_node);
    if (new_node == NULL) {
        return false;
    }
 
    new_node->callback = handler;
    new_node->definition = definition;
    new_node->user_data = user_data;
    new_node->timeout = timeout;
    new_node->interval = interval;
    new_node->timer_id = timer_id;
 
    if (definition == NULL || definition->enabled == true) {
        new_node->fd = timerfd_create(CLOCK_REALTIME, 0);
        if (new_node->fd == -1) {
            free(new_node);
            MYMPD_LOG_ERROR("Can't create timerfd");
            return false;
        }
 
        struct itimerspec new_value;
        //timeout
        new_value.it_value.tv_sec = timeout;
        new_value.it_value.tv_nsec = 0;
        //interval
        //0 = one shote and deactivate
        //-1 = one shot and remove
        
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
    if (definition == NULL || definition->enabled == true) {
        l->active++;
    }
    
    MYMPD_LOG_DEBUG("Added timer with id %d, start time in %ds", timer_id, timeout);
    
    return true;
}
 
void remove_timer(struct t_timer_list *l, int timer_id) {
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
            if (current->definition == NULL || current->definition->enabled == true) {
                l->active--;
            }
            free_timer_node(current);
            return;
        }
    }
}

void toggle_timer(struct t_timer_list *l, int timer_id) {
    struct t_timer_node *current = NULL;
    for (current = l->list; current != NULL; current = current->next) {
        if (current->timer_id == timer_id && current->definition != NULL) {
            current->definition->enabled = current->definition->enabled == true ? false : true;
            return;
        }
    }
}

void truncate_timerlist(struct t_timer_list *l) {
    struct t_timer_node *current = l->list;
    struct t_timer_node *tmp = NULL;
    
    while (current != NULL) {
        MYMPD_LOG_DEBUG("Removing timer with id %d", current->timer_id);
        tmp = current;
        current = current->next;
        free_timer_node(tmp);
    }
    init_timerlist(l);
}

void free_timer_definition(struct t_timer_definition *timer_def) {
    sdsfree(timer_def->name);
    sdsfree(timer_def->action);
    sdsfree(timer_def->subaction);
    sdsfree(timer_def->playlist);
    list_free(&timer_def->arguments);
    FREE_PTR(timer_def);
}

void free_timer_node(struct t_timer_node *node) {
    if (node->fd > -1) {
        close(node->fd);
    }
    if (node->definition != NULL) {
        free_timer_definition(node->definition);
    }
    free(node);
}

bool free_timerlist(struct t_timer_list *l) {
    struct t_timer_node *current = l->list;
    struct t_timer_node *tmp = NULL;
    while (current != NULL) {
        tmp = current->next;
        free_timer_node(current);
        current = tmp;
    }
    init_timerlist(l);
    return true;
}

struct t_timer_definition *parse_timer(struct t_timer_definition *timer_def, const char *str, size_t len) {
    char *name = NULL;
    bool enabled;
    int start_hour;
    int start_minute;
    int volume;
    unsigned jukebox_mode;
    char *action = NULL;
    char *subaction = NULL;
    char *playlist = NULL;
    int je = json_scanf(str, len, "{params: {name: %Q, enabled: %B, startHour: %d, startMinute: %d, action: %Q, subaction: %Q, volume: %d, playlist: %Q, jukeboxMode: %u}}",
        &name, &enabled, &start_hour, &start_minute, &action, &subaction, &volume, &playlist, &jukebox_mode);
    if (je == 9 || (je == 8 && subaction == NULL)) {
        if (start_hour < 0 || start_hour > 23 || start_minute < 0 || start_minute > 59) {
            start_hour = 0;
            start_minute = 0;        
        }
        MYMPD_LOG_DEBUG("Successfully parsed timer definition");
        timer_def->name = sdsnew(name);
        timer_def->enabled = enabled;
        timer_def->start_hour = start_hour;
        timer_def->start_minute = start_minute;
        if (je == 8) {
            //pre 6.5.0 timer definition
            if (strcmp(action, "startplay") == 0 || strcmp(action, "stopplay") == 0) {
                timer_def->action = sdsnew("player");
            }
            else {
                timer_def->action = sdsnew("syscmd");
            }
            timer_def->subaction = sdsnew(action);
        }
        else {
            timer_def->action = sdsnew(action);
            timer_def->subaction = sdsnew(subaction);
        }
        timer_def->volume = volume;
        timer_def->playlist = sdsnew(playlist);
        timer_def->jukebox_mode = jukebox_mode;
        list_init(&timer_def->arguments);
        void *h = NULL;
        struct json_token key;
        struct json_token val;
        while ((h = json_next_key(str, (int)strlen(str), h, ".params.arguments", &key, &val)) != NULL) {
            list_push_len(&timer_def->arguments, key.ptr, key.len, 0, val.ptr, val.len, NULL);
        }
        
        for (int i = 0; i < 7; i++) {
            timer_def->weekdays[i] = false;
        }
        struct json_token t;
        for (int i = 0; json_scanf_array_elem(str, len, ".params.weekdays", i, &t) > 0 && i < 7; i++) {
            timer_def->weekdays[i] = t.type == JSON_TYPE_TRUE ? true : false;
        }
    }
    else {
        MYMPD_LOG_ERROR("Error parsing timer definition");
        free(timer_def);
        timer_def = NULL;
    }
    
    FREE_PTR(name);
    FREE_PTR(action);
    FREE_PTR(subaction);
    FREE_PTR(playlist);
    
    return timer_def;
}

time_t timer_calc_starttime(int start_hour, int start_minute, int interval) {
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

sds timer_list(struct t_mympd_state *mympd_state, sds buffer, sds method, long request_id) {
    buffer = jsonrpc_result_start(buffer, method, request_id);
    buffer = sdscat(buffer, "\"data\":[");
    int entities_returned = 0;
    struct t_timer_node *current = mympd_state->timer_list.list;
    while (current != NULL) {
        if (current->timer_id > 99 && current->definition != NULL) {
            if (entities_returned++) {
                buffer = sdscatlen(buffer, ",", 1);
            }
            buffer = sdscatlen(buffer, "{", 1);
            buffer = tojson_long(buffer, "timerid", current->timer_id, true);
            buffer = tojson_long(buffer, "interval", current->interval, true);
            buffer = tojson_char(buffer, "name", current->definition->name, true);
            buffer = tojson_bool(buffer, "enabled", current->definition->enabled, true);
            buffer = tojson_long(buffer, "startHour", current->definition->start_hour, true);
            buffer = tojson_long(buffer, "startMinute", current->definition->start_minute, true);
            buffer = tojson_char(buffer, "action", current->definition->action, true);
            buffer = tojson_char(buffer, "subaction", current->definition->subaction, true);
            buffer = tojson_char(buffer, "playlist", current->definition->playlist, true);
            buffer = tojson_long(buffer, "volume", current->definition->volume, true);
            buffer = tojson_long(buffer, "jukeboxMode", current->definition->jukebox_mode, true);
            buffer = sdscat(buffer, "\"weekdays\":[");
            for (int i = 0; i < 7; i++) {
                if (i > 0) {
                    buffer = sdscatlen(buffer, ",", 1);
                }
                buffer = sdscat(buffer, current->definition->weekdays[i] == true ? "true" : "false");
            }
            buffer = sdscatlen(buffer, "]}", 2);
        }
        current = current->next;
    }
    
    buffer = sdscat(buffer, "],");
    buffer = tojson_long(buffer, "returnedEntities", entities_returned, false);
    buffer = jsonrpc_result_end(buffer);
    return buffer;
}

sds timer_get(struct t_mympd_state *mympd_state, sds buffer, sds method, long request_id, int timer_id) {
    buffer = jsonrpc_result_start(buffer, method, request_id);
    bool found = false;
    struct t_timer_node *current = mympd_state->timer_list.list;
    while (current != NULL) {
        if (current->timer_id == timer_id && current->definition != NULL) {
            buffer = tojson_long(buffer, "timerid", current->timer_id, true);
            buffer = tojson_char(buffer, "name", current->definition->name, true);
            buffer = tojson_long(buffer, "interval", current->interval, true);
            buffer = tojson_bool(buffer, "enabled", current->definition->enabled, true);
            buffer = tojson_long(buffer, "startHour", current->definition->start_hour, true);
            buffer = tojson_long(buffer, "startMinute", current->definition->start_minute, true);
            buffer = tojson_char(buffer, "action", current->definition->action, true);
            buffer = tojson_char(buffer, "subaction", current->definition->subaction, true);
            buffer = tojson_char(buffer, "playlist", current->definition->playlist, true);
            buffer = tojson_long(buffer, "volume", current->definition->volume, true);
            buffer = tojson_long(buffer, "jukeboxMode", current->definition->jukebox_mode, true);
            buffer = sdscat(buffer, "\"weekdays\":[");
            for (int i = 0; i < 7; i++) {
                if (i > 0) {
                    buffer = sdscatlen(buffer, ",", 1);
                }
                buffer = sdscat(buffer, current->definition->weekdays[i] == true ? "true" : "false");
            }
            buffer = sdscat(buffer, "],\"arguments\": {");
            struct list_node *argument = current->definition->arguments.head;
            int i = 0;
            while (argument != NULL) {
                if (i++) {
                    buffer = sdscatlen(buffer, ",", 1);
                }
                buffer = tojson_char(buffer, argument->key, argument->value_p, false);
                argument = argument->next;            
            }
            buffer = sdscatlen(buffer, "}", 1);
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

bool timerfile_read(struct t_mympd_state *mympd_state) {
    sds timer_file = sdscatfmt(sdsempty(), "%s/state/timer_list", mympd_state->config->workdir);
    char *line = NULL;
    size_t n = 0;
    errno = 0;
    FILE *fp = fopen(timer_file, "r");
    if (fp != NULL) {
        while (getline(&line, &n, fp) > 0) {
            struct t_timer_definition *timer_def = malloc(sizeof(struct t_timer_definition));
            assert(timer_def);
            sds param = sdscatfmt(sdsempty(), "{params: %s}", line);
            timer_def = parse_timer(timer_def, param, sdslen(param));
            int interval;
            int timerid;
            int je = json_scanf(param, sdslen(param), "{params: {interval: %d, timerid: %d}}", &interval, &timerid);
            sdsfree(param);
            
            if (je == 2 && timer_def != NULL) {
                if (timerid > mympd_state->timer_list.last_id) {
                    mympd_state->timer_list.last_id = timerid;
                }
                time_t start = timer_calc_starttime(timer_def->start_hour, timer_def->start_minute, interval);
                add_timer(&mympd_state->timer_list, start, interval, timer_handler_select, timerid, timer_def, NULL);
            }
            else {
                MYMPD_LOG_ERROR("Invalid timer line");
                MYMPD_LOG_DEBUG("Errorneous line: %s", line);
            }
        }
        FREE_PTR(line);
        fclose(fp);
    }
    else {
        //ignore error
        MYMPD_LOG_DEBUG("Can not open file \"%s\"", timer_file);
        MYMPD_LOG_ERRNO(errno);
    }
    sdsfree(timer_file);
    MYMPD_LOG_INFO("Read %d timer(s) from disc", mympd_state->timer_list.length);
    return true;
}

bool timerfile_save(struct t_mympd_state *mympd_state) {
    MYMPD_LOG_INFO("Saving timers to disc");
    sds tmp_file = sdscatfmt(sdsempty(), "%s/state/timer_list.XXXXXX", mympd_state->config->workdir);
    errno = 0;
    int fd = mkstemp(tmp_file);
    if (fd < 0) {
        MYMPD_LOG_ERROR("Can not open file \"%s\" for write", tmp_file);
        MYMPD_LOG_ERRNO(errno);
        sdsfree(tmp_file);
        return false;
    }
    FILE *fp = fdopen(fd, "w");
    struct t_timer_node *current = mympd_state->timer_list.list;
    sds buffer = sdsempty();
    while (current != NULL) {
        if (current->timer_id > 99 && current->definition != NULL) {
            buffer = sdsreplace(buffer, "{");
            buffer = tojson_long(buffer, "timerid", current->timer_id, true);
            buffer = tojson_long(buffer, "interval", current->interval, true);
            buffer = tojson_char(buffer, "name", current->definition->name, true);
            buffer = tojson_bool(buffer, "enabled", current->definition->enabled, true);
            buffer = tojson_long(buffer, "startHour", current->definition->start_hour, true);
            buffer = tojson_long(buffer, "startMinute", current->definition->start_minute, true);
            buffer = tojson_char(buffer, "action", current->definition->action, true);
            buffer = tojson_char(buffer, "subaction", current->definition->subaction, true);
            buffer = tojson_char(buffer, "playlist", current->definition->playlist, true);
            buffer = tojson_long(buffer, "volume", current->definition->volume, true);
            buffer = tojson_long(buffer, "jukeboxMode", current->definition->jukebox_mode, true);
            buffer = sdscat(buffer, "\"weekdays\":[");
            for (int i = 0; i < 7; i++) {
                if (i > 0) {
                    buffer = sdscatlen(buffer, ",", 1);
                }
                buffer = sdscat(buffer, current->definition->weekdays[i] == true ? "true" : "false");
            }
            buffer = sdscat(buffer, "],\"arguments\": {");
            struct list_node *argument = current->definition->arguments.head;
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
        }
        current = current->next;
    }
    fclose(fp);
    sdsfree(buffer);
    sds timer_file = sdscatfmt(sdsempty(), "%s/state/timer_list", mympd_state->config->workdir);
    errno = 0;
    if (rename(tmp_file, timer_file) == -1) {
        MYMPD_LOG_ERROR("Renaming file from \"%s\" to \"%s\" failed", tmp_file, timer_file);
        MYMPD_LOG_ERRNO(errno);
        sdsfree(tmp_file);
        sdsfree(timer_file);
        return false;
    }
    sdsfree(tmp_file);
    sdsfree(timer_file);
    return true;    
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
