/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2022 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#ifndef MYMPD_API_LAST_PLAYED_H
#define MYMPD_API_LAST_PLAYED_H

#include "../lib/mympd_state.h"

bool mympd_api_last_played_add_song(struct t_mympd_state *mympd_state, const int song_id);
bool mympd_api_last_played_file_save(struct t_list *last_played, long last_played_count, sds workdir);
sds mympd_api_last_played_list(struct t_mympd_state *mympd_state, sds buffer,
        long request_id, long offset, long limit, sds searchstr, const struct t_tags *tagcols);
#endif
