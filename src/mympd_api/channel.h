/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2024 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

/*! \file
 * \brief myMPD channel API
 */

#ifndef MYMPD_API_CHANNEL_H
#define MYMPD_API_CHANNEL_H

#include "src/lib/mympd_state.h"

sds mympd_api_channel_list(struct t_partition_state *partition_state, sds buffer, unsigned request_id);
sds mympd_api_channel_messages_read(struct t_partition_state *partition_state, sds buffer, unsigned request_id);

#endif
