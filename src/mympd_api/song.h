/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2024 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

/*! \file
 * \brief myMPD song API
 */

#ifndef MYMPD_API_SONG_H
#define MYMPD_API_SONG_H

#include "src/lib/mympd_state.h"

sds mympd_api_song_details(struct t_mympd_state *mympd_state, struct t_partition_state *partition_state,
    sds buffer, unsigned request_id, const char *uri);
sds mympd_api_song_comments(struct t_partition_state *partition_state, sds buffer, unsigned request_id, const char *uri);
#endif
