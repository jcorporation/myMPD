/*
 SPDX-License-Identifier: GPL-2.0-or-later
 myMPD (c) 2018-2020 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#ifndef __MPD_WORKER_SMARTPLS_H__
#define __MPD_WORKER_SMARTPLS_H__
bool mpd_worker_smartpls_update_all(t_config *config, t_mpd_worker_state *mpd_worker_state);
bool mpd_worker_smartpls_update(t_config *config, t_mpd_worker_state *mpd_worker_state, const char *playlist);
#endif
