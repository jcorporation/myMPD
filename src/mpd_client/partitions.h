/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2025 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

/*! \file
 * \brief MPD partitions specific functions
 */

#ifndef MYMPD_MPD_CLIENT_PARTITIONS_H
#define MYMPD_MPD_CLIENT_PARTITIONS_H

#include "src/lib/mympd_state.h"

bool partitions_connect(struct t_mympd_state *mympd_state, struct t_partition_state *partition_state);
struct t_partition_state *partitions_get_by_name(struct t_mympd_state *mympd_state, const char *name);
void partitions_list_clear(struct t_mympd_state *mympd_state);
bool partitions_populate(struct t_mympd_state *mympd_state);
bool partitions_check(struct t_mympd_state *mympd_state, const char *name);
void partitions_add(struct t_mympd_state *mympd_state, const char *name);
#endif
