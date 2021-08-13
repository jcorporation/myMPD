/*
 SPDX-License-Identifier: GPL-2.0-or-later
 myMPD (c) 2018-2021 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#ifndef MYMPD_MPD_WORKER_SMARTPLS_H
#define MYMPD_MPD_WORKER_SMARTPLS_H

#include "mpd_worker_utility.h"

#include <stdbool.h>

bool mpd_worker_smartpls_update_all(struct t_mpd_worker_state *mpd_worker_state, bool force);
bool mpd_worker_smartpls_update(struct t_mpd_worker_state *mpd_worker_state, const char *playlist);
#endif
