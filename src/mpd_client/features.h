/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2024 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

/*! \file
 * \brief MPD feature detection
 */

#ifndef MYMPD_MPD_CLIENT_FEATURES_H
#define MYMPD_MPD_CLIENT_FEATURES_H

#include "src/lib/mympd_state.h"

void mpd_client_mpd_features(struct t_mympd_state *mympd_state, struct t_partition_state *partition_state);
#endif
