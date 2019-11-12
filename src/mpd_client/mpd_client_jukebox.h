/*
 SPDX-License-Identifier: GPL-2.0-or-later
 myMPD (c) 2018-2019 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#ifndef __JUKEBOX_H__
#define __JUKEBOX_H__
bool mpd_client_jukebox(t_mpd_state *mpd_state);
bool mpd_client_jukebox_add(t_mpd_state *mpd_state, const int addSongs, const enum jukebox_modes jukebox_mode, const char *jukebox_playlist);
#endif
