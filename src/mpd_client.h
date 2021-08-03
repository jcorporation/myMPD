/*
 SPDX-License-Identifier: GPL-2.0-or-later
 myMPD (c) 2018-2021 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#ifndef __MPD_CLIENT_H__
#define __MPD_CLIENT_H__

#include "mympd_state.h"

void mpd_client_parse_idle(struct t_mympd_state *mympd_state, int idle_bitmask);
void mpd_client_idle(struct t_mympd_state *mympd_state);
#endif
