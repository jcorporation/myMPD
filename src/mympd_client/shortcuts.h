/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2025 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

/*! \file
 * \brief Some MPD shortcuts
 */

#ifndef MYMPD_MPD_CLIENT_SHORTCUTS_H
#define MYMPD_MPD_CLIENT_SHORTCUTS_H

#include "src/lib/mympd_state.h"

bool mympd_client_command_list_end_check(struct t_partition_state *partition_state);
bool mympd_client_add_uris_to_queue(struct t_partition_state *partition_state, struct t_list *uris,
        unsigned to, unsigned whence, sds *error);
bool mympd_client_add_album_to_queue(struct t_partition_state *partition_state, struct t_cache *album_cache,
    sds album_id, unsigned to, unsigned whence, sds *error);
bool mympd_client_add_albums_to_queue(struct t_partition_state *partition_state, struct t_cache *album_cache,
    struct t_list *albumids, unsigned to, unsigned whence, sds *error);

#endif
