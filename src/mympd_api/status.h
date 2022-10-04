/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2022 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#ifndef MYMPD_API_STATUS_H
#define MYMPD_API_STATUS_H

#include "../lib/mympd_state.h"

unsigned mympd_api_get_elapsed_seconds(struct mpd_status *status);
sds mympd_api_status_print(struct t_partition_state *partition_state, sds buffer, struct mpd_status *status);
sds mympd_api_status_updatedb_state(struct t_partition_state *partition_state, sds buffer);
long mympd_api_status_updatedb_id(struct t_partition_state *partition_state);
sds mympd_api_status_volume_get(struct t_partition_state *partition_state, sds buffer, long request_id);

sds mympd_api_status_get(struct t_partition_state *partition_state, sds buffer, long request_id);
sds mympd_api_status_current_song(struct t_partition_state *partition_state, sds buffer, long request_id);

bool mympd_api_status_lua_mympd_state_set(struct t_list *lua_partition_state, struct t_partition_state *partition_state);
#endif
