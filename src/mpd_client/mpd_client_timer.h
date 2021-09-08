/*
 SPDX-License-Identifier: GPL-2.0-or-later
 myMPD (c) 2018-2021 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#ifndef MYMPD_MPD_CLIENT_TIMER_H
#define MYMPD_MPD_CLIENT_TIMER_H

#include "../../dist/src/sds/sds.h"
#include "../lib/api.h"
#include "../lib/mympd_state.h"

sds mpd_client_timer_startplay(struct t_mympd_state *mympd_state, sds buffer, sds method, long request_id, 
                               unsigned volume, const char *playlist, enum jukebox_modes jukebox_mode);
#endif
