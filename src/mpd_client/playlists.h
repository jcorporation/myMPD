/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2023 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#ifndef MYMPD_MPD_CLIENT_PLAYLISTS_H
#define MYMPD_MPD_CLIENT_PLAYLISTS_H

#include "src/lib/mympd_state.h"

enum playlist_types {
    PLTYPE_ALL = 0,
    PLTYPE_STATIC = 1,
    PLTYPE_SMART = 2,
    PLTYPE_SMARTPLS_ONLY = 3
};

bool mpd_client_playlist_shuffle(struct t_partition_state *partition_state, const char *uri);
bool mpd_client_playlist_sort(struct t_partition_state *partition_state, const char *uri, const char *tagstr);
time_t mpd_client_get_playlist_mtime(struct t_partition_state *partition_state, const char *playlist);
long mpd_client_enum_playlist(struct t_partition_state *partition_state, const char *playlist, bool empty_check);
long mpd_client_playlist_validate(struct t_partition_state *partition_state, const char *playlist, bool remove);
long mpd_client_playlist_dedup(struct t_partition_state *partition_state, const char *playlist, bool remove);
#endif
