/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2024 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

/*! \file
 * \brief Functions to populate the jukebox queue
 */

#ifndef MYMPD_MPD_WORKER_JUKEBOX_H
#define MYMPD_MPD_WORKER_JUKEBOX_H

#include "src/mpd_worker/state.h"

bool mpd_worker_jukebox_push(struct t_mpd_worker_state *mpd_worker_state);
bool mpd_worker_jukebox_error(struct t_mpd_worker_state *mpd_worker_state, sds error);
bool mpd_worker_jukebox_queue_fill(struct t_mpd_worker_state *mpd_worker_state, struct t_list *queue_list,
        unsigned add_songs, sds *error);
bool mpd_worker_jukebox_queue_fill_add(struct t_mpd_worker_state *mpd_worker_state, struct t_list *queue_list,
        unsigned add_songs, sds *error);

#endif
