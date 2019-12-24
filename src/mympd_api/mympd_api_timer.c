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

#include "../log.h"
#include "mympd_api_timer.h"

//private definitions
#define MAX_TIMER_COUNT 100

static struct t_timer_node *get_timer_from_fd(struct t_timer_list *l, int fd);

//public functions
void init_timerlist(struct t_timer_list *l) {
    l->length = 0;
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
                    current->callback(current->user_data);
                }
                if (current->type == TIMER_SINGLE_SHOT) {
                    remove_timer(l, current->timer_id);
                }
            }
        }
    }
    return;
}

bool add_timer(struct t_timer_list *l, unsigned int interval, time_handler handler, t_timer type, int timer_id, void *user_data) {
    struct t_timer_node *new_node = (struct t_timer_node *)malloc(sizeof(struct t_timer_node));
    if (new_node == NULL) {
        return false;
    }
 
    new_node->callback  = handler;
    new_node->user_data = user_data;
    new_node->interval  = interval;
    new_node->type      = type;
    new_node->timer_id  = timer_id;
 
    new_node->fd = timerfd_create(CLOCK_REALTIME, 0);
    if (new_node->fd == -1) {
        free(new_node);
        LOG_ERROR("Can't create timerfd");
        return false;
    }
 
    struct itimerspec new_value;
    new_value.it_value.tv_sec = interval;
    new_value.it_value.tv_nsec = 0;
 
    if (type == TIMER_PERIODIC) {
        new_value.it_interval.tv_sec = interval;
    }
    else {
        new_value.it_interval.tv_sec = 0;
    }
    new_value.it_interval.tv_nsec = 0;

    timerfd_settime(new_node->fd, 0, &new_value, NULL);
 
    /*Inserting the timer node into the list*/
    new_node->next = l->list;
    l->list = new_node;
    l->length++;

    return true;
}
 
void remove_timer(struct t_timer_list *l, int timer_id) {
    struct t_timer_node *current;
    struct t_timer_node *previous;
    
    for (current = l->list; current != NULL; previous = current, current = current->next) {
        if (current->timer_id == timer_id) {
            LOG_DEBUG("Removing timer with id %d", timer_id);
            if (previous == NULL) {
                // Fix beginning pointer
                l->list = current->next;
            } else {
                // Fix previous node's next to skip over the removed node.
                previous->next = current->next;
            }
            // Deallocate the node
            close(current->fd);
            free(current);
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
        close(tmp->fd);
        free(tmp);
    }
    init_timerlist(l);
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
