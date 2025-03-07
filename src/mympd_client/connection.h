/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2025 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

/*! \file
 * \brief MPD connection handling
 */

#ifndef MYMPD_MPD_CLIENT_CONNECTION_H
#define MYMPD_MPD_CLIENT_CONNECTION_H

#include "src/lib/mympd_state.h"

bool mympd_client_connect(struct t_partition_state *partition_state);
bool mympd_client_set_connection_options(struct t_partition_state *partition_state);
void mympd_client_disconnect(struct t_partition_state *partition_state);
void mympd_client_disconnect_silent(struct t_partition_state *partition_state);
void mympd_client_disconnect_all(struct t_mympd_state *mympd_state);
#endif
