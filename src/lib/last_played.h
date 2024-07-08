/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2024 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

/*! \file
 * \brief Last played implementation
 */

#ifndef MYMPD_LAST_PLAYED_H
#define MYMPD_LAST_PLAYED_H

#include "src/lib/mympd_state.h"

bool last_played_file_save(struct t_partition_state *partition_state);
bool last_played_file_read(struct t_partition_state *partition_state);

#endif
