/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2025 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

/*! \file
 * \brief Enumeration of MPD playlists
 */

#ifndef MYMPD_MPD_WORKER_PLAYLISTS_H
#define MYMPD_MPD_WORKER_PLAYLISTS_H

#include "src/mympd_worker/state.h"

sds mympd_worker_playlist_content_enumerate(struct t_partition_state *partition_state, sds buffer, unsigned request_id, sds plist);

#endif
