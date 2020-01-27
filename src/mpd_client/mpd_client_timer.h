/*
 SPDX-License-Identifier: GPL-2.0-or-later
 myMPD (c) 2018-2020 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#ifndef __MPD_CLIENT_TIMER_H__
#define __MPD_CLIENT_TIMER_H__
void mpd_client_set_timer(enum mympd_cmd_ids cmd_id, const char *cmd, int timeout, int interval, const char *handler);
sds mpd_client_timer_startplay(t_mpd_state *mpd_state, sds buffer, sds method, int request_id, 
                               unsigned volume, const char *playlist, enum jukebox_modes jukebox_mode);
#endif
