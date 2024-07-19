/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2024 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

/*! \file
 * \brief Add random functions
 */

#ifndef MYMPD_MPD_WORKER_ADD_RANDOM_H
#define MYMPD_MPD_WORKER_ADD_RANDOM_H

#include "src/mpd_worker/state.h"

bool mpd_worker_add_random_to_queue(struct t_mpd_worker_state *mpd_worker_state,
        unsigned add, unsigned mode, sds plist, bool play, sds partition);

#endif
