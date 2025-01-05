/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2025 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

/*! \file
 * \brief myMPD mounts API
 */

#ifndef MYMPD_API_MOUNTS_H
#define MYMPD_API_MOUNTS_H

#include "src/lib/mympd_state.h"

sds mympd_api_mounts_list(struct t_partition_state *partition_state, sds buffer, unsigned request_id);
sds mympd_api_mounts_neighbor_list(struct t_partition_state *partition_state, sds buffer, unsigned request_id);
sds mympd_api_mounts_urlhandler_list(struct t_partition_state *partition_state, sds buffer, unsigned request_id);
#endif
