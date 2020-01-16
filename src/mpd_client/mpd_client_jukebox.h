/*
 SPDX-License-Identifier: GPL-2.0-or-later
 myMPD (c) 2018-2020 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#ifndef __JUKEBOX_H__
#define __JUKEBOX_H__
bool mpd_client_jukebox(t_config *config, t_mpd_state *mpd_state);
bool mpd_client_jukebox_add_to_queue(t_config *config, t_mpd_state *mpd_state, int addSongs, enum jukebox_modes jukebox_mode, const char *playlist, bool manual);
#endif
