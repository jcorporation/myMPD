/*
 SPDX-License-Identifier: GPL-2.0-or-later
 myMPD (c) 2018-2020 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#ifndef __MPD_SHARED_PLAYLISTS_H__
#define __MPD_SHARED_PLAYLISTS_H__
sds mpd_shared_playlist_shuffle_sort(t_mpd_state *mpd_state, sds buffer, sds method, long request_id, const char *uri, const char *tagstr);
bool mpd_shared_smartpls_save(t_config *config, const char *smartpltype, 
                              const char *playlist, const char *tag, const char *searchstr, const int maxentries, 
                              const int timerange, const char *sort);
unsigned long mpd_shared_get_playlist_mtime(t_mpd_state *mpd_state, const char *playlist);
unsigned long mpd_shared_get_smartpls_mtime(t_config *config, const char *playlist);
unsigned long mpd_shared_get_db_mtime(t_mpd_state *mpd_state);
#endif
