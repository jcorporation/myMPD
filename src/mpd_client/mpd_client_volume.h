/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2022 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#ifndef MYMPD_MPD_CLIENT_VOLUME_H
#define MYMPD_MPD_CLIENT_VOLUME_H

#include "../lib/mympd_state.h"

int mpd_client_get_volume(struct t_mpd_state *mpd_state);
#endif
