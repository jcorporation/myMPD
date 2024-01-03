/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2024 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#ifndef MPD_WORKER_SONG_H
#define MPD_WORKER_SONG_H

#include "src/lib/mympd_state.h"

sds mpd_worker_song_fingerprint(struct t_partition_state *partition_state, sds buffer, unsigned request_id, const char *uri);
#endif
