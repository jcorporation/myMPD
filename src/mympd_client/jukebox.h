/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2025 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

/*! \file
 * \brief Jukebox functions
 */

#ifndef MYMPD_MPDCLIENT_JUKEBOX_H
#define MYMPD_MPDCLIENT_JUKEBOX_H

#include "src/lib/mympd_state.h"

void jukebox_clear_all(struct t_mympd_state *mympd_state);
void jukebox_disable(struct t_partition_state *partition_state);
bool jukebox_trigger_script(struct t_mympd_state *mympd_state, struct t_partition_state *partition_state,
        bool add_to_queue);
bool jukebox_run(struct t_mympd_state *mympd_state, struct t_partition_state *partition_state,
    struct t_cache *album_cache);

bool jukebox_add_to_queue(struct t_partition_state *partition_state,
        struct t_cache *album_cache, unsigned add_songs, sds *error);

#endif
