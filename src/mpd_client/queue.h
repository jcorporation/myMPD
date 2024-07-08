/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2024 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

/*! \file
 * \brief MPD queue functions.
 */

#ifndef MYMPD_MPD_CLIENT_QUEUE_H
#define MYMPD_MPD_CLIENT_QUEUE_H

#include "src/lib/mympd_state.h"

bool mpd_client_queue_play_newly_inserted(struct t_partition_state *partition_state, sds *error);
bool mpd_client_queue_check_start_play(struct t_partition_state *partition_state, bool play, sds *error);
bool mpd_client_queue_clear(struct t_partition_state *partition_state, sds *error);
void mpd_client_queue_status_update(struct t_partition_state *partition_state);
sds mpd_client_queue_status_print(struct t_partition_state *partition_state, struct t_cache *album_cache, sds buffer);

#endif
