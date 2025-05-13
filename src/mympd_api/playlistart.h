/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2025 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

/*! \file
 * \brief myMPD playlistart API
 */

#ifndef MYMPD_API_PLAYLISTART_H
#define MYMPD_API_PLAYLISTART_H

#include "src/lib/mympd_state.h"

sds mympd_api_playlistart(struct t_mympd_state *mympd_state, struct t_partition_state *partition_state,
        sds buffer, unsigned request_id, unsigned long conn_id, sds name, sds type);

#endif
