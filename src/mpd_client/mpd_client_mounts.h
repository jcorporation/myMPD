/*
 SPDX-License-Identifier: GPL-2.0-or-later
 myMPD (c) 2018-2020 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#ifndef __MPD_CLIENT_MOUNTS_H__
#define __MPD_CLIENT_MOUNTS_H__
sds mpd_client_put_mounts(t_mpd_state *mpd_state, sds buffer, sds method, int request_id);
sds mpd_client_put_neighbors(t_mpd_state *mpd_state, sds buffer, sds method, int request_id);
sds mpd_client_put_urlhandlers(t_mpd_state *mpd_state, sds buffer, sds method, int request_id);
#endif
