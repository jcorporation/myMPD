/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2025 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

/*! \file
 * \brief myMPD timer handlers
 */

#ifndef MYMPD_API_TIMER_HANDLERS_H
#define MYMPD_API_TIMER_HANDLERS_H

#include "src/lib/mympd_state.h"

/**
 * Internal timer ids
 */
enum timer_ids {
    TIMER_ID_DISK_CACHE_CROP = 1,
    TIMER_ID_SMARTPLS_UPDATE = 2,
    TIMER_ID_CACHES_CREATE = 3,
    TIMER_ID_WEBRADIODB_UPDATE = 4
};

void timer_handler_by_id(unsigned timer_id, struct t_timer_definition *definition);
void timer_handler_select(unsigned timer_id, struct t_timer_definition *definition);
bool mympd_api_timer_startplay(struct t_partition_state *partition_state,
        unsigned volume, sds playlist, sds preset);
#endif
