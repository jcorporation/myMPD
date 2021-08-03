/*
 SPDX-License-Identifier: GPL-2.0-or-later
 myMPD (c) 2018-2021 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#ifndef __MPD_SHARED_PLAYLISTS_H__
#define __MPD_SHARED_PLAYLISTS_H__

#include "../mympd_state.h"

sds mpd_shared_playlist_shuffle_sort(struct t_mpd_state *mpd_state, sds buffer, sds method, 
                                     long request_id, const char *uri, const char *tagstr);
bool mpd_shared_smartpls_save(const char *workdir, const char *smartpltype, 
                              const char *playlist, const char *expression,
                              const int maxentries, const int timerange, const char *sort);
unsigned long mpd_shared_get_playlist_mtime(struct t_mpd_state *mpd_state, const char *playlist);
unsigned long mpd_shared_get_smartpls_mtime(struct t_config *config, const char *playlist);
unsigned long mpd_shared_get_db_mtime(struct t_mpd_state *mpd_state);
#endif
