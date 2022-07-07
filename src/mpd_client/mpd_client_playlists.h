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

bool is_smartpls(sds workdir, const char *playlist);
bool mpd_client_replace_playlist(struct t_mpd_state *mpd_state, const char *new_pl,
        const char *to_replace_pl, const char *backup_pl);
sds mpd_client_playlist_shuffle(struct t_mpd_state *mpd_state, sds buffer, sds method,
        long request_id, const char *uri);
sds mpd_client_playlist_sort(struct t_mpd_state *mpd_state, sds buffer, sds method,
        long request_id, const char *uri, const char *tagstr);
bool mpd_client_smartpls_save(sds workdir, const char *smartpltype,
        const char *playlist, const char *expression, const int maxentries,
        const int timerange, const char *sort);
time_t mpd_client_get_playlist_mtime(struct t_mpd_state *mpd_state, const char *playlist);
time_t mpd_client_get_smartpls_mtime(struct t_config *config, const char *playlist);
time_t mpd_client_get_db_mtime(struct t_mpd_state *mpd_state);
#endif
