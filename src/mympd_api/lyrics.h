/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2025 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

/*! \file
 * \brief myMPD lyrics API
 */

#ifndef MYMPD_API_LYRICS_H
#define MYMPD_API_LYRICS_H

#include "src/lib/mympd_state.h"

sds mympd_api_lyrics_get(struct t_mympd_state *mympd_state, sds buffer,
        sds uri, sds partition, unsigned long conn_id, unsigned request_id);
#endif
