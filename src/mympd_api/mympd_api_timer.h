/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2022 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#ifndef MYMPD_API_TIMER_H
#define MYMPD_API_TIMER_H

#include "../../dist/sds/sds.h"
#include "../lib/mympd_state.h"

enum timer_intervals {
    TIMER_ONE_SHOT_REMOVE = -1,
    TIMER_ONE_SHOT_DISABLE = 0
};

void mympd_api_timer_timerlist_init(struct t_timer_list *l);
void mympd_api_timer_timerlist_clear(struct t_timer_list *l);
void mympd_api_timer_check(struct t_timer_list *l);
bool mympd_api_timer_add(struct t_timer_list *l, time_t timeout, int interval,
    timer_handler handler, int timer_id, struct t_timer_definition *definition);
bool mympd_api_timer_replace(struct t_timer_list *l, time_t timeout, int interval,
    timer_handler handler, int timer_id, struct t_timer_definition *definition);
void mympd_api_timer_remove(struct t_timer_list *l, int timer_id);
void mympd_api_timer_toggle(struct t_timer_list *l, int timer_id);
void mympd_api_timer_free_definition(struct t_timer_definition *timer_def);
void mympd_api_timer_free_node(struct t_timer_node *node);
struct t_timer_definition *mympd_api_timer_parse(struct t_timer_definition *timer_def, sds str,
    sds *error);
time_t mympd_api_timer_calc_starttime(int start_hour, int start_minute, int interval);
sds mympd_api_timer_list(struct t_mympd_state *mympd_state, sds buffer, sds method, long request_id);
sds mympd_api_timer_get(struct t_mympd_state *mympd_state, sds buffer, sds method, long request_id,
    int timer_id);
bool mympd_api_timer_file_read(struct t_mympd_state *mympd_state);
bool mympd_api_timer_file_save(struct t_mympd_state *mympd_state);
#endif
