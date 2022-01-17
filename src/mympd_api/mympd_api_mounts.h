/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2022 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#ifndef MYMPD_API_MOUNTS_H
#define MYMPD_API_MOUNTS_H

#include "../lib/mympd_state.h"

sds mympd_api_mounts_list(struct t_mympd_state *mympd_state, sds buffer, sds method, long request_id);
sds mympd_api_mounts_neighbor_list(struct t_mympd_state *mympd_state, sds buffer, sds method, long request_id);
sds mympd_api_mounts_urlhandler_list(struct t_mympd_state *mympd_state, sds buffer, sds method, long request_id);
#endif
