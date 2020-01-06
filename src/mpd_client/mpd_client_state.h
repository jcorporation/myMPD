/*
 SPDX-License-Identifier: GPL-2.0-or-later
 myMPD (c) 2018-2020 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#ifndef __MPD_CLIENT_STATE_H__
#define __MPD_CLIENT_STATE_H__
sds mpd_client_get_updatedb_state(t_mpd_state *mpd_state, sds buffer);
sds mpd_client_put_state(t_config *config, t_mpd_state *mpd_state, sds buffer, sds method, int request_id);
sds mpd_client_put_volume(t_mpd_state *mpd_state, sds buffer, sds method, int request_id);
sds mpd_client_put_outputs(t_mpd_state *mpd_state, sds buffer, sds method, int request_id);
sds mpd_client_put_current_song(t_mpd_state *mpd_state, sds buffer, sds method, int request_id);
#endif
