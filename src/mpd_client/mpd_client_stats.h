/*
 SPDX-License-Identifier: GPL-2.0-or-later
 myMPD (c) 2018-2019 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#ifndef __MPD_CLIENT_STATS_H__
#define __MPD_CLIENT_STATS_H__
bool mpd_client_count_song_uri(t_mpd_state *mpd_state, const char *uri, const char *name, const int value);
sds mpd_client_like_song_uri(t_mpd_state *mpd_state, sds buffer, sds method, int request_id,
                             const char *uri, int value);
bool mpd_client_last_played_song_uri(t_mpd_state *mpd_state, const char *uri);
bool mpd_client_last_skipped_song_uri(t_mpd_state *mpd_state, const char *uri);
bool mpd_client_last_played_list(t_config *config, t_mpd_state *mpd_state, const int song_id);
bool mpd_client_last_played_list_save(t_config *config, t_mpd_state *mpd_state);
sds mpd_client_put_last_played_songs(t_config *config, t_mpd_state *mpd_state, sds buffer, sds method, int request_id, 
                                     unsigned int offset, const t_tags *tagcols);
sds mpd_client_put_stats(t_config *config, t_mpd_state *mpd_state, sds buffer, sds method, int request_id);
#endif
