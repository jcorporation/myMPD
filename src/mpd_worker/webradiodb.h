/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2024 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#ifndef MPD_WORKER_WEBRADIODB_H
#define MPD_WORKER_WEBRADIODB_H

#include "src/mpd_worker/state.h"

#include <stdbool.h>

bool mpd_worker_webradiodb_update(struct t_mpd_worker_state *mpd_worker_state);

#endif
