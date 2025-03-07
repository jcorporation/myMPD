/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2025 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

/*! \file
 * \brief API handler for mympd_worker thread
 */

#ifndef MYMPD_MPD_WORKER_API_H
#define MYMPD_MPD_WORKER_API_H

#include "src/mympd_worker/state.h"

void mympd_worker_api(struct t_mympd_worker_state *mympd_worker_state);
#endif
