/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2025 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

/*! \file
 * \brief MPD client idle event handling
 */

#ifndef MYMPD_MPD_CLIENT_IDLE_H
#define MYMPD_MPD_CLIENT_IDLE_H

#include "src/lib/api.h"
#include "src/lib/mympd_state.h"

void mympd_client_idle(struct t_mympd_state *mympd_state, struct t_work_request *request);
void mympd_client_scrobble(struct t_mympd_state *mympd_state, struct t_partition_state *partition_state);
#endif
