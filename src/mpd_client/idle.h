/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2023 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#ifndef MYMPD_MPD_CLIENT_IDLE_H
#define MYMPD_MPD_CLIENT_IDLE_H

#include "src/lib/api.h"
#include "src/lib/mympd_state.h"

void mpd_client_idle(struct t_mympd_state *mympd_state, struct t_work_request *request);
#endif
