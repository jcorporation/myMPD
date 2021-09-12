/*
 SPDX-License-Identifier: GPL-2.0-or-later
 myMPD (c) 2018-2021 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#ifndef MYMPD_API_PARTITIONS_H
#define MYMPD_API_PARTITIONS_H

#include "../lib/mympd_state.h"

sds mympd_api_get_partitions(struct t_mympd_state *mympd_state, sds buffer, sds method, long request_id);
#endif
