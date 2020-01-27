/*
 SPDX-License-Identifier: GPL-2.0-or-later
 myMPD (c) 2018-2020 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#ifndef MYMPD_API_TIMER_H
#define MYMPD_API_TIMER_H
void init_timerlist(struct t_timer_list *l);
void truncate_timerlist(struct t_timer_list *l);
void check_timer(struct t_timer_list *l, bool gui);
bool add_timer(struct t_timer_list *l, unsigned int timeout, unsigned int interval, time_handler handler, int timer_id, 
               struct t_timer_definition *definition, void *user_data);
bool replace_timer(struct t_timer_list *l, unsigned int timeout, unsigned int interval, time_handler handler, int timer_id, 
                   struct t_timer_definition *definition, void *user_data);
void remove_timer(struct t_timer_list *l, int timer_id);
void toggle_timer(struct t_timer_list *l, int timer_id);
void free_timer_definition(struct t_timer_definition *timer_def);
void free_timer_node(struct t_timer_node *node);
struct t_timer_definition *parse_timer(struct t_timer_definition *timer_def, const char *str, size_t len);
time_t timer_calc_starttime(int start_hour, int start_minute);
sds timer_list(t_mympd_state *mympd_state, sds buffer, sds method, int request_id);
sds timer_get(t_mympd_state *mympd_state, sds buffer, sds method, int request_id, int timer_id);
bool timerfile_read(t_config *config, t_mympd_state *mympd_state);
bool timerfile_save(t_config *config, t_mympd_state *mympd_state);
#endif
