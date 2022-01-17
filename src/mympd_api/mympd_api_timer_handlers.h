/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2022 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#ifndef MYMPD_API_TIMER_HANDLERS_H
#define MYMPD_API_TIMER_HANDLERS_H

#include "../lib/mympd_state.h"

void timer_handler_covercache(struct t_timer_definition *definition, void *user_data); //timer_id 1
void timer_handler_smartpls_update(struct t_timer_definition *definition, void *user_data); //timer_id 2
void timer_handler_select(struct t_timer_definition *definition, void *user_data); // for all gui timers
sds mympd_api_timer_startplay(struct t_mympd_state *mympd_state, sds buffer, sds method, long request_id,
                               unsigned volume, const char *playlist, enum jukebox_modes jukebox_mode);
#endif
