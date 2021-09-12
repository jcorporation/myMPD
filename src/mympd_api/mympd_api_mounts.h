/*
 SPDX-License-Identifier: GPL-2.0-or-later
 myMPD (c) 2018-2021 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#ifndef MYMPD_API_MOUNTS_H
#define MYMPD_API_MOUNTS_H

#include "../lib/mympd_state.h"

sds mympd_api_get_mounts(struct t_mympd_state *mympd_state, sds buffer, sds method, long request_id);
sds mympd_api_get_neighbors(struct t_mympd_state *mympd_state, sds buffer, sds method, long request_id);
sds mympd_api_get_urlhandlers(struct t_mympd_state *mympd_state, sds buffer, sds method, long request_id);
#endif
