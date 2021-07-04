/*
 SPDX-License-Identifier: GPL-2.0-or-later
 myMPD (c) 2018-2021 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#ifndef __MPD_CLIENT_STATE_H__
#define __MPD_CLIENT_STATE_H__
sds mpd_client_get_updatedb_state(struct t_mympd_state *mympd_state, sds buffer);
long mpd_client_get_updatedb_id(struct t_mympd_state *mympd_state);
sds mpd_client_put_state(struct t_mympd_state *mympd_state, sds buffer, sds method, long request_id);
sds mpd_client_put_volume(struct t_mympd_state *mympd_state, sds buffer, sds method, long request_id);
sds mpd_client_put_outputs(struct t_mympd_state *mympd_state, sds buffer, sds method, long request_id);
sds mpd_client_put_partition_outputs(struct t_mympd_state *mympd_state, sds buffer, sds method, long request_id,
                                     const char *partition);
sds mpd_client_put_current_song(struct t_mympd_state *mympd_state, sds buffer, sds method, long request_id);
bool mpd_client_get_lua_mympd_state(struct t_mympd_state *mympd_state, struct list *lua_mympd_state);
#endif
