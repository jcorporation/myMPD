/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2023 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#ifndef MYMPD_API_LAST_PLAYED_H
#define MYMPD_API_LAST_PLAYED_H

#include "src/lib/mympd_state.h"

bool mympd_api_last_played_add_song(struct t_partition_state *partition_state, long last_played_count, int song_id);
sds mympd_api_last_played_list(struct t_partition_state *partition_state, struct t_partition_state *stickerdb,
        sds buffer, long request_id, long offset, long limit, sds expression, const struct t_tags *tagcols);
#endif
