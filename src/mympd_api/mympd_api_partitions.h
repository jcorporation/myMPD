/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2022 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#ifndef MYMPD_API_PARTITIONS_H
#define MYMPD_API_PARTITIONS_H

#include "../lib/mympd_state.h"

sds mympd_api_partition_list(struct t_mympd_state *mympd_state, sds buffer, sds method, long request_id);
#endif
