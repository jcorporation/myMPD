/*
 SPDX-License-Identifier: GPL-2.0-or-later
 myMPD (c) 2018-2021 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#ifndef MYMPD_API_TIMER_H
#define MYMPD_API_TIMER_H

#include "../../dist/src/sds/sds.h"
#include "../lib/mympd_state.h"

void init_timerlist(struct t_timer_list *l);
void truncate_timerlist(struct t_timer_list *l);
void check_timer(struct t_timer_list *l);
bool add_timer(struct t_timer_list *l, unsigned int timeout, int interval, time_handler handler, int timer_id, 
               struct t_timer_definition *definition, void *user_data);
bool replace_timer(struct t_timer_list *l, unsigned int timeout, int interval, time_handler handler, int timer_id, 
                   struct t_timer_definition *definition, void *user_data);
void remove_timer(struct t_timer_list *l, int timer_id);
void toggle_timer(struct t_timer_list *l, int timer_id);
void free_timer_definition(struct t_timer_definition *timer_def);
void free_timer_node(struct t_timer_node *node);
bool free_timerlist(struct t_timer_list *l);
struct t_timer_definition *parse_timer(struct t_timer_definition *timer_def, const char *str, size_t len);
time_t timer_calc_starttime(int start_hour, int start_minute, int interval);
sds timer_list(struct t_mympd_state *mympd_state, sds buffer, sds method, long request_id);
sds timer_get(struct t_mympd_state *mympd_state, sds buffer, sds method, long request_id, int timer_id);
bool timerfile_read(struct t_mympd_state *mympd_state);
bool timerfile_save(struct t_mympd_state *mympd_state);
#endif
