/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2022 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#ifndef MYMPD_API_STATS_H
#define MYMPD_API_STATS_H

#include "../lib/mympd_state.h"

bool mympd_api_stats_last_played_add_song(struct t_mympd_state *mympd_state, const int song_id);
bool mympd_api_stats_last_played_file_save(struct t_mympd_state *mympd_state);
sds mympd_api_stats_last_played_list(struct t_mympd_state *mympd_state, sds buffer, sds method,
                                     long request_id, const unsigned offset,
                                     const unsigned limit, sds searchstr, const struct t_tags *tagcols);
sds mympd_api_stats_get(struct t_mympd_state *mympd_state, sds buffer, sds method, long request_id);
#endif
