/*
 SPDX-License-Identifier: GPL-2.0-or-later
 myMPD (c) 2018-2020 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#ifndef __JUKEBOX_H__
#define __JUKEBOX_H__
bool mpd_client_rm_jukebox_entry(t_mpd_client_state *mpd_client_state, unsigned pos);
sds mpd_client_put_jukebox_list(t_mpd_client_state *mpd_client_state, sds buffer, sds method, long request_id, 
                                unsigned int offset, const t_tags *tagcols);
bool mpd_client_jukebox(t_config *config, t_mpd_client_state *mpd_client_state, unsigned attempt);
bool mpd_client_jukebox_add_to_queue(t_config *config, t_mpd_client_state *mpd_client_state, unsigned add_songs, enum jukebox_modes jukebox_mode, const char *playlist, bool manual);
#endif
