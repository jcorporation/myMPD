/*
 SPDX-License-Identifier: GPL-2.0-or-later
 myMPD (c) 2018-2021 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#ifndef MYMPD_MPD_CLIENT_MOUNTS_H
#define MYMPD_MPD_CLIENT_MOUNTS_H

#include "../lib/mympd_state.h"

sds mpd_client_get_mounts(struct t_mympd_state *mympd_state, sds buffer, sds method, long request_id);
sds mpd_client_get_neighbors(struct t_mympd_state *mympd_state, sds buffer, sds method, long request_id);
sds mpd_client_get_urlhandlers(struct t_mympd_state *mympd_state, sds buffer, sds method, long request_id);
#endif
