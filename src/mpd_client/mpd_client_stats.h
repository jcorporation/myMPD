/*
 SPDX-License-Identifier: GPL-2.0-or-later
 myMPD (c) 2018-2020 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#ifndef __MPD_CLIENT_STATS_H__
#define __MPD_CLIENT_STATS_H__
bool mpd_client_last_played_list(t_config *config, t_mpd_state *mpd_state, const int song_id);
bool mpd_client_last_played_list_save(t_config *config, t_mpd_state *mpd_state);
sds mpd_client_put_last_played_songs(t_config *config, t_mpd_state *mpd_state, sds buffer, sds method, int request_id, 
                                     unsigned int offset, const t_tags *tagcols);
sds mpd_client_put_stats(t_config *config, t_mpd_state *mpd_state, sds buffer, sds method, int request_id);
#endif
