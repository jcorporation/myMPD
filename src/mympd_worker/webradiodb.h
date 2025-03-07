/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2025 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

/*! \file
 * \brief WebradioDB update functions
 */

#ifndef MPD_WORKER_WEBRADIODB_H
#define MPD_WORKER_WEBRADIODB_H

#include "src/mympd_worker/state.h"

#include <stdbool.h>

bool mympd_worker_webradiodb_update(struct t_mympd_worker_state *mympd_worker_state, bool force);

#endif
