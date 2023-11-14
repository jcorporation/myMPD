/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2023 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#ifndef MYMPD_MPD_CLIENT_SHORTCUTS_H
#define MYMPD_MPD_CLIENT_SHORTCUTS_H

#include "src/lib/mympd_state.h"

bool mpd_client_command_list_end_check(struct t_partition_state *partition_state);

bool mpd_client_add_album_to_queue(struct t_partition_state *partition_state, struct t_cache *album_cache,
    sds album_id, unsigned to, unsigned whence, sds *error);

#endif
