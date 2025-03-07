/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2025 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

/*! \file
 * \brief Add random functions
 */

#ifndef MYMPD_MPD_WORKER_ADD_RANDOM_H
#define MYMPD_MPD_WORKER_ADD_RANDOM_H

#include "src/mympd_worker/state.h"

bool mympd_worker_add_random_to_queue(struct t_mympd_worker_state *mympd_worker_state,
        unsigned add, unsigned mode, sds plist, bool play, sds partition);
sds mympd_worker_list_random(struct t_mympd_worker_state *mympd_worker_state, sds buffer, unsigned request_id,
        unsigned quantity, unsigned mode, sds plist);

#endif
