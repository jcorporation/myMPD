/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2022 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#ifndef MYMPD_API_SONG_H
#define MYMPD_API_SONG_H

#include "src/lib/mympd_state.h"

sds mympd_api_song_fingerprint(struct t_partition_state *partition_state, sds buffer, long request_id, const char *uri);
sds mympd_api_song_details(struct t_partition_state *partition_state, sds buffer, long request_id, const char *uri);
sds mympd_api_song_comments(struct t_partition_state *partition_state, sds buffer, long request_id, const char *uri);
#endif
