/*
 SPDX-License-Identifier: GPL-2.0-or-later
 myMPD (c) 2018-2021 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#ifndef MYMPD_API_STATUS_H
#define MYMPD_API_STATUS_H

#include "../lib/mympd_state.h"

sds mympd_api_get_updatedb_state(struct t_mympd_state *mympd_state, sds buffer);
long mympd_api_get_updatedb_id(struct t_mympd_state *mympd_state);
sds mympd_api_get_status(struct t_mympd_state *mympd_state, sds buffer, sds method, long request_id);
sds mympd_api_get_volume(struct t_mympd_state *mympd_state, sds buffer, sds method, long request_id);
sds mympd_api_get_outputs(struct t_mympd_state *mympd_state, sds buffer, sds method, long request_id);
sds mympd_api_get_partition_outputs(struct t_mympd_state *mympd_state, sds buffer, sds method, long request_id,
                                     const char *partition);
sds mympd_api_get_current_song(struct t_mympd_state *mympd_state, sds buffer, sds method, long request_id);
bool mympd_api_get_lua_mympd_state(struct t_mympd_state *mympd_state, struct t_list *lua_mympd_state);
#endif
