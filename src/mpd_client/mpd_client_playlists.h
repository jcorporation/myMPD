/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2022 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#ifndef MYMPD_MPD_CLIENT_PLAYLISTS_H
#define MYMPD_MPD_CLIENT_PLAYLISTS_H

#include "../lib/mympd_state.h"

enum playlist_types {
    PLTYPE_ALL = 0,
    PLTYPE_STATIC = 1,
    PLTYPE_SMART = 2,
    PLTYPE_SMARTPLS_ONLY = 3
};

bool mpd_client_playlist_shuffle(struct t_mpd_state *mpd_state, const char *uri);
bool mpd_client_playlist_sort(struct t_mpd_state *mpd_state, const char *uri, const char *tagstr);
time_t mpd_client_get_playlist_mtime(struct t_mpd_state *mpd_state, const char *playlist);
time_t mpd_client_get_db_mtime(struct t_mpd_state *mpd_state);
int mpd_client_enum_playlist(struct t_mympd_state *mympd_state, const char *playlist, bool empty_check);
#endif
