/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2024 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

/*! \file
 * \brief Album cache creation
 */

#ifndef MYMPD_MPD_WORKER_ALBUM_CACHE_H
#define MYMPD_MPD_WORKER_ALBUM_CACHE_H

#include "src/mpd_worker/state.h"

bool mpd_worker_album_cache_create(struct t_mpd_worker_state *mpd_worker_state, bool force);
#endif
