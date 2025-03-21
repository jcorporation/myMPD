/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2025 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

/*! \file
 * \brief myMPD volume API
 */

#ifndef MYMPD_API_VOLUME_H
#define MYMPD_API_VOLUME_H

#include "dist/sds/sds.h"
#include "src/lib/api.h"
#include "src/lib/mympd_state.h"

sds mympd_api_volume_set(struct t_partition_state *partition_state, unsigned volume_min, unsigned volume_max,
        sds buffer, enum mympd_cmd_ids cmd_id, unsigned request_id, unsigned volume);
sds mympd_api_volume_change(struct t_partition_state *partition_state, unsigned volume_min, unsigned volume_max,
        sds buffer, unsigned request_id, int relative_volume);

#endif
