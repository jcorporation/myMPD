/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2022 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#ifndef MYMPD_API_VOLUME_H
#define MYMPD_API_VOLUME_H

#include "dist/sds/sds.h"
#include "src/lib/mympd_state.h"

sds mympd_api_volume_set(struct t_partition_state *partition_state, sds buffer, long request_id, unsigned volume);
sds mympd_api_volume_change(struct t_partition_state *partition_state, sds buffer, long request_id, int relative_volume);

#endif
