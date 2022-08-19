/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2022 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#ifndef MYMPD_API_PARTITIONS_H
#define MYMPD_API_PARTITIONS_H

#include "../lib/mympd_state.h"

struct t_partition_state *mympd_api_get_partition_by_name(struct t_mympd_state *mympd_state, const char *partition);
sds mympd_api_partition_list(struct t_partition_state *partition_state, sds buffer, long request_id);
#endif
