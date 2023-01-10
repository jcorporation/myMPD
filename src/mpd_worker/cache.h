/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2023 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#ifndef MYMPD_MPD_WORKER_CACHE_H
#define MYMPD_MPD_WORKER_CACHE_H

#include "src/mpd_worker/state.h"

bool mpd_worker_cache_init(struct t_mpd_worker_state *mpd_worker_state, bool force);
#endif
