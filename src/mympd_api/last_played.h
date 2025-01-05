/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2025 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

/*! \file
 * \brief myMPD last played API
 */

#ifndef MYMPD_API_LAST_PLAYED_H
#define MYMPD_API_LAST_PLAYED_H

#include "src/lib/mympd_state.h"

bool mympd_api_last_played_add_song(struct t_partition_state *partition_state, unsigned last_played_count);
sds mympd_api_last_played_list(struct t_partition_state *partition_state, struct t_stickerdb_state *stickerdb,
        sds buffer, unsigned request_id, unsigned offset, unsigned limit, sds expression, const struct t_fields *tagcols);
#endif
