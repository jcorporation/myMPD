/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2024 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#ifndef MYMPD_MPD_WORKER_PLAYLISTS_H
#define MYMPD_MPD_WORKER_PLAYLISTS_H

#include "src/mpd_worker/state.h"

sds mpd_worker_playlist_content_enumerate(struct t_partition_state *partition_state, sds buffer, unsigned request_id, sds plist);

#endif
