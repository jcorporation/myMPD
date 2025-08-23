/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2025 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

/*! \file
 * \brief Compatibility functions for older libmpdclient versions
 */

#ifndef MYMPD_MPD_CLIENT_COMPAT_H
#define MYMPD_MPD_CLIENT_COMPAT_H

#include "src/lib/mympd_state.h"

bool mympd_client_stringnormalization_all(struct t_partition_state *partition_state);
bool mympd_client_stringnormalization_clear(struct t_partition_state *partition_state);

#endif
