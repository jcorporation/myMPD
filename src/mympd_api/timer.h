/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2024 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#ifndef MYMPD_API_TIMER_H
#define MYMPD_API_TIMER_H

#include "dist/sds/sds.h"
#include "src/lib/jsonrpc.h"
#include "src/lib/mympd_state.h"

enum timer_intervals {
    TIMER_ONE_SHOT_REMOVE = -1,
    TIMER_ONE_SHOT_DISABLE = 0
};

/**
 * Callback functions for timers
 */
typedef void (*timer_handler)(unsigned timer_id, struct t_timer_definition *definition);

/**
 * Timer node
 */
struct t_timer_node {
    int fd;                                 //!< holds the timerfd
    timer_handler callback;                 //!< timer callback function
    struct t_timer_definition *definition;  //!< optional pointer to timer definition (GUI)
    int timeout;                            //!< seconds when timer will run
    int interval;                           //!< reschedule timer interval
};

void mympd_api_timer_timerlist_init(struct t_timer_list *l);
void mympd_api_timer_timerlist_clear(struct t_timer_list *l);
bool mympd_api_timer_check(int fd, struct t_timer_list *timer_list);
bool mympd_api_timer_save(struct t_partition_state *partition_state, struct t_timer_list *timer_list, int interval, unsigned timerid,
        struct t_timer_definition *timer_def, sds *error);
bool mympd_api_timer_add(struct t_timer_list *l, int timeout, int interval,
        timer_handler handler, unsigned timer_id, struct t_timer_definition *definition);
bool mympd_api_timer_replace(struct t_timer_list *l, int timeout, int interval,
        timer_handler handler, unsigned timer_id, struct t_timer_definition *definition);
bool mympd_api_timer_remove(struct t_timer_list *l, unsigned timer_id);
bool mympd_api_timer_toggle(struct t_timer_list *l, unsigned timer_id, sds *error);
void *mympd_api_timer_free_definition(struct t_timer_definition *timer_def);
struct t_timer_definition *mympd_api_timer_parse(sds str, const char *partition, struct t_jsonrpc_parse_error *error);
int mympd_api_timer_calc_starttime(int start_hour, int start_minute, int interval);
sds mympd_api_timer_list(struct t_timer_list *timer_list, sds buffer, unsigned request_id, const char *partition);
sds mympd_api_timer_get(struct t_timer_list *timer_list, sds buffer, unsigned request_id, unsigned timer_id);
bool mympd_api_timer_file_read(struct t_timer_list *timer_list, sds workdir);
bool mympd_api_timer_file_save(struct t_timer_list *timer_list, sds workdir);
#endif
