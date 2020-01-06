/*
 SPDX-License-Identifier: GPL-2.0-or-later
 myMPD (c) 2018-2019 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <string.h>
#include <sys/timerfd.h>
#include <pthread.h>
#include <poll.h>
#include <stdio.h>
#include <stdbool.h>
#include <errno.h>
#include <time.h>

#include "../../dist/src/frozen/frozen.h"
#include "../../dist/src/sds/sds.h"
#include "../sds_extras.h"
#include "../log.h"
#include "../list.h"
#include "config_defs.h"
#include "../utility.h"
#include "mympd_api_utility.h"
#include "mympd_api_timer.h"
#include "mympd_api_timer_handlers.h"

//private definitions
#define MAX_TIMER_COUNT 100

static struct t_timer_node *get_timer_from_fd(struct t_timer_list *l, int fd);

//public functions
void init_timerlist(struct t_timer_list *l) {
    l->length = 0;
    l->last_id = 100;
    l->list = NULL;
}

void check_timer(struct t_timer_list *l) {
    int iMaxCount = 0;
    struct t_timer_node *current = l->list;
    uint64_t exp;

    struct pollfd ufds[MAX_TIMER_COUNT] = {{0}};
    memset(ufds, 0, sizeof(struct pollfd) * MAX_TIMER_COUNT);
    while (current != NULL) {
        ufds[iMaxCount].fd = current->fd;
        ufds[iMaxCount].events = POLLIN;
        iMaxCount++;
        current = current->next;
    }

    int read_fds = poll(ufds, iMaxCount, 100);
    if (read_fds < 0) {
        LOG_ERROR("Error polling timerfd: %s", strerror(errno));
        return;
    }
    else if (read_fds == 0) {
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
                LOG_DEBUG("Timer with id %d triggered", current->timer_id);
                if (current->callback) {
                    current->callback(current->definition, current->user_data);
                }
                if (current->interval == 0) {
                    remove_timer(l, current->timer_id);
                }
            }
        }
    }
    return;
}

bool replace_timer(struct t_timer_list *l, unsigned int timeout, unsigned int interval, time_handler handler, 
                   int timer_id, struct t_timer_definition *definition, void *user_data)
{
    remove_timer(l, timer_id);
    return add_timer(l, timeout, interval, handler, timer_id, definition, user_data);
}

bool add_timer(struct t_timer_list *l, unsigned int timeout, unsigned int interval, time_handler handler, 
               int timer_id, struct t_timer_definition *definition, void *user_data) 
{
    struct t_timer_node *new_node = (struct t_timer_node *)malloc(sizeof(struct t_timer_node));
    if (new_node == NULL) {
        return false;
    }
 
    new_node->callback = handler;
    new_node->definition = definition;
    new_node->user_data = user_data;
    new_node->timeout = timeout;
    new_node->interval = interval;
    new_node->timer_id = timer_id;
 
    new_node->fd = timerfd_create(CLOCK_REALTIME, 0);
    if (new_node->fd == -1) {
        free(new_node);
        LOG_ERROR("Can't create timerfd");
        return false;
    }
 
    struct itimerspec new_value;
    //timeout
    new_value.it_value.tv_sec = timeout;
    new_value.it_value.tv_nsec = 0;
    //interval, 0 = single shot timer
    new_value.it_interval.tv_sec = interval;
    new_value.it_interval.tv_nsec = 0;

    timerfd_settime(new_node->fd, 0, &new_value, NULL);
 
    //Inserting the timer node into the list
    new_node->next = l->list;
    l->list = new_node;
    l->length++;

    return true;
}
 
void remove_timer(struct t_timer_list *l, int timer_id) {
    struct t_timer_node *current = NULL;
    struct t_timer_node *previous = NULL;
    
    for (current = l->list; current != NULL; previous = current, current = current->next) {
        if (current->timer_id == timer_id) {
            LOG_DEBUG("Removing timer with id %d", timer_id);
            if (previous == NULL) {
                //Fix beginning pointer
                l->list = current->next;
            } else {
                //Fix previous nodes next to skip over the removed node.
                previous->next = current->next;
            }
            //Deallocate the node
            free_timer_node(current);
            return;
        }
    }
}

void toggle_timer(struct t_timer_list *l, int timer_id) {
    struct t_timer_node *current = NULL;
    for (current = l->list; current != NULL; current = current->next) {
        if (current->timer_id == timer_id) {
            current->definition->enabled = current->definition->enabled == true ? false : true;
            return;
        }
    }
}

void truncate_timerlist(struct t_timer_list *l) {
    struct t_timer_node *current = l->list;
    struct t_timer_node *tmp = NULL;
    
    while (current != NULL) {
        LOG_DEBUG("Removing timer with id %d", current->timer_id);
        tmp = current;
        current = current->next;
        free_timer_node(tmp);
    }
    init_timerlist(l);
}

void free_timer_node(struct t_timer_node *node) {
    close(node->fd);
    if (node->definition != NULL) {
        sdsfree(node->definition->name);
        sdsfree(node->definition->action);
        sdsfree(node->definition->playlist);
        free(node->definition);
    }
    free(node);
}

struct t_timer_definition *parse_timer(struct t_timer_definition *timer_def, const char *str, size_t len) {
    char *name = NULL;
    char *action = NULL;
    char *playlist = NULL;
    int je = json_scanf(str, len, "{params: {name: %Q, enabled: %B, startHour: %d, startMinute: %d, action: %Q, volume: %d, playlist: %Q}}",
        &name, &timer_def->enabled, &timer_def->start_hour, &timer_def->start_minute, &action, &timer_def->volume, &playlist);
    if (je == 7) {
        timer_def->name = sdsnew(name);
        timer_def->action = sdsnew(action);
        timer_def->playlist = sdsnew(playlist);
    }
    FREE_PTR(name);
    FREE_PTR(action);
    FREE_PTR(playlist);
    if (je != 7) {
        return NULL;
    }
    struct json_token t;
    for (int i = 0; json_scanf_array_elem(str, len, ".params.weekdays", i, &t) > 0 && i < 7; i++) {
        timer_def->weekdays[i] = t.type == JSON_TYPE_TRUE ? true : false;
    }
    return timer_def;
}

time_t timer_calc_starttime(int start_hour, int start_minute) {
    time_t now = time(NULL);
    struct tm tms;
    localtime_r(&now, &tms);
    tms.tm_hour = start_hour;
    tms.tm_min = start_minute;
    tms.tm_sec = 0;
    return mktime(&tms);
}

sds timer_list(t_mympd_state *mympd_state, sds buffer, sds method, int request_id) {
    buffer = jsonrpc_start_result(buffer, method, request_id);
    buffer = sdscat(buffer, ",\"data\":[");
    int entities_returned = 0;
    struct t_timer_node *current = mympd_state->timer_list.list;
    while (current != NULL) {
        if (current->timer_id > 99) {
            if (entities_returned++) {
                buffer = sdscatlen(buffer, ",", 1);
            }
            buffer = sdscatlen(buffer, "{", 1);
            buffer = tojson_long(buffer, "timerid", current->timer_id, true);
            buffer = tojson_char(buffer, "name", current->definition->name, true);
            buffer = tojson_bool(buffer, "enabled", current->definition->enabled, true);
            buffer = tojson_long(buffer, "startHour", current->definition->start_hour, true);
            buffer = tojson_long(buffer, "startMinute", current->definition->start_minute, true);
            buffer = tojson_char(buffer, "action", current->definition->action, true);
            buffer = tojson_char(buffer, "playlist", current->definition->playlist, true);
            buffer = tojson_long(buffer, "volume", current->definition->volume, true);
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
    buffer = jsonrpc_end_result(buffer);
    return buffer;
}

sds timer_get(t_mympd_state *mympd_state, sds buffer, sds method, int request_id, int timer_id) {
    buffer = jsonrpc_start_result(buffer, method, request_id);
    buffer = sdscat(buffer, ",");
    struct t_timer_node *current = mympd_state->timer_list.list;
    while (current != NULL) {
        if (current->timer_id == timer_id) {
            buffer = tojson_long(buffer, "timerid", current->timer_id, true);
            buffer = tojson_char(buffer, "name", current->definition->name, true);
            buffer = tojson_bool(buffer, "enabled", current->definition->enabled, true);
            buffer = tojson_long(buffer, "startHour", current->definition->start_hour, true);
            buffer = tojson_long(buffer, "startMinute", current->definition->start_minute, true);
            buffer = tojson_char(buffer, "action", current->definition->action, true);
            buffer = tojson_char(buffer, "playlist", current->definition->playlist, true);
            buffer = tojson_long(buffer, "volume", current->definition->volume, true);
            buffer = sdscat(buffer, "\"weekdays\":[");
            for (int i = 0; i < 7; i++) {
                if (i > 0) {
                    buffer = sdscatlen(buffer, ",", 1);
                }
                buffer = sdscat(buffer, current->definition->weekdays[i] == true ? "true" : "false");
            }
            buffer = sdscatlen(buffer, "]", 1);
            break;
        }
        current = current->next;
    }
    
    buffer = jsonrpc_end_result(buffer);
    return buffer;
}

bool timerfile_read(t_config *config, t_mympd_state *mympd_state) {
    sds timer_file = sdscatfmt(sdsempty(), "%s/state/timer", config->varlibdir);
    char *line = NULL;
    size_t n = 0;
    ssize_t read = 0;
    FILE *fp = fopen(timer_file, "r");
    sdsfree(timer_file);
    if (fp != NULL) {
        while ((read = getline(&line, &n, fp)) > 0) {
            struct t_timer_definition *timer_def = malloc(sizeof(struct t_timer_definition));
            sds param = sdscatfmt(sdsempty(), "{params: %s}", line);
            timer_def = parse_timer(timer_def, param, sdslen(param));
            int timerid;
            int je = json_scanf(param, sdslen(param), "{params: {timerid: %d}}", &timerid);
            sdsfree(param);
            if (je == 1 && timer_def != NULL) {
                time_t start = timer_calc_starttime(timer_def->start_hour, timer_def->start_minute);
                add_timer(&mympd_state->timer_list, start, 86400, timer_handler_select, timerid, timer_def, NULL);
            }
            else {
                LOG_ERROR("Invalid timer line");
            }
        }
        FREE_PTR(line);
        fclose(fp);
    }
    LOG_VERBOSE("Read %d timer(s) from disc", mympd_state->timer_list.length);
    return true;
}

bool timerfile_save(t_config *config, t_mympd_state *mympd_state) {
    if (config->readonly == true) {
        return true;
    }
    LOG_VERBOSE("Saving timers to disc");
    sds tmp_file = sdscatfmt(sdsempty(), "%s/state/timer.XXXXXX", config->varlibdir);
    int fd = mkstemp(tmp_file);
    if (fd < 0) {
        LOG_ERROR("Can't open %s for write", tmp_file);
        sdsfree(tmp_file);
        return false;
    }
    FILE *fp = fdopen(fd, "w");
    struct t_timer_node *current = mympd_state->timer_list.list;
    sds buffer = sdsempty();
    while (current != NULL) {
        if (current->timer_id > 99) {
            buffer = sdsreplace(buffer, "{");
            buffer = tojson_long(buffer, "timerid", current->timer_id, true);
            buffer = tojson_char(buffer, "name", current->definition->name, true);
            buffer = tojson_bool(buffer, "enabled", current->definition->enabled, true);
            buffer = tojson_long(buffer, "startHour", current->definition->start_hour, true);
            buffer = tojson_long(buffer, "startMinute", current->definition->start_minute, true);
            buffer = tojson_char(buffer, "action", current->definition->action, true);
            buffer = tojson_char(buffer, "playlist", current->definition->playlist, true);
            buffer = tojson_long(buffer, "volume", current->definition->volume, true);
            buffer = sdscat(buffer, "\"weekdays\":[");
            for (int i = 0; i < 7; i++) {
                if (i > 0) {
                    buffer = sdscatlen(buffer, ",", 1);
                }
                buffer = sdscat(buffer, current->definition->weekdays[i] == true ? "true" : "false");
            }
            buffer = sdscatlen(buffer, "]}\n", 3);
            fputs(buffer, fp);
        }
        current = current->next;
    }
    fclose(fp);
    sdsfree(buffer);
    sds timer_file = sdscatfmt(sdsempty(), "%s/state/timer", config->varlibdir);
    if (rename(tmp_file, timer_file) == -1) {
        LOG_ERROR("Renaming file from %s to %s failed", tmp_file, timer_file);
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
