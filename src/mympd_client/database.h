/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2025 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

/*! \file
 * \brief MPD database helper functions
 */

#ifndef MYMPD_MPD_CLIENT_DATABASE_H
#define MYMPD_MPD_CLIENT_DATABASE_H

#include "src/lib/mympd_state.h"

time_t mympd_client_get_db_mtime(struct t_partition_state *partition_state);
bool mympd_client_song_exists(struct t_partition_state *partition_state, const char *uri);

#endif
