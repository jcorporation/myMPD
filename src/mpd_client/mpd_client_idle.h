/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2022 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#ifndef MYMPD_MPD_CLIENT_LOOP_H
#define MYMPD_MPD_CLIENT_LOOP_H

#include "../lib/mympd_state.h"

void mpd_client_parse_idle(struct t_mympd_state *mympd_state, unsigned idle_bitmask);
void mpd_client_idle(struct t_mympd_state *mympd_state);
#endif
