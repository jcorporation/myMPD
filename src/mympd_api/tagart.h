/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2025 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

/*! \file
 * \brief myMPD tagart API
 */

#ifndef MYMPD_API_TAGART_H
#define MYMPD_API_TAGART_H

#include "src/lib/mympd_state.h"

sds mympd_api_tagart(struct t_mympd_state *mympd_state, struct t_partition_state *partition_state,
        sds buffer, unsigned request_id, unsigned long conn_id, sds tag, sds value);

#endif
