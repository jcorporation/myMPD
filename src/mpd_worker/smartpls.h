/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2025 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

/*! \file
 * \brief Smart playlist generation
 */

#ifndef MYMPD_MPD_WORKER_SMARTPLS_H
#define MYMPD_MPD_WORKER_SMARTPLS_H

#include "src/mpd_worker/state.h"

#include <stdbool.h>

bool mpd_worker_smartpls_update_all(struct t_mpd_worker_state *mpd_worker_state, bool force);
bool mpd_worker_smartpls_update(struct t_mpd_worker_state *mpd_worker_state, const char *playlist);
#endif
