/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2023 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#ifndef MYMPD_API_PARTITIONS_H
#define MYMPD_API_PARTITIONS_H

#include "src/lib/mympd_state.h"

sds mympd_api_partition_list(struct t_mympd_state *mympd_state, sds buffer, unsigned request_id);
bool mympd_api_partition_new(struct t_partition_state *partition_state, sds partition, sds *error);
bool mympd_api_partition_outputs_move(struct t_partition_state *partition_state, struct t_list *outputs, sds *error);
sds mympd_api_partition_rm(struct t_mympd_state *mympd_state, struct t_partition_state *partition_state, sds buffer, unsigned request_id, sds partition);

#endif
