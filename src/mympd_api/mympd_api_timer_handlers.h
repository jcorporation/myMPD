/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2022 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#ifndef MYMPD_API_TIMER_HANDLERS_H
#define MYMPD_API_TIMER_HANDLERS_H

#include "../lib/mympd_state.h"

//internal timers
enum timer_ids {
    TIMER_ID_COVERCACHE = 1,
    TIMER_ID_SMARTPLS_UPDATE = 2
};

void timer_handler_by_id(int timer_id, struct t_timer_definition *definition, void *user_data);

//user defined timers
void timer_handler_select(int timer_id, struct t_timer_definition *definition, void *user_data);
sds mympd_api_timer_startplay(struct t_mympd_state *mympd_state, sds buffer, sds method, long request_id,
        unsigned volume, sds playlist, enum jukebox_modes jukebox_mode);
#endif
