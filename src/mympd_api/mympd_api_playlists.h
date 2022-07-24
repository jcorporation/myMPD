/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2022 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#ifndef MYMPD_API_PLAYLISTS_H
#define MYMPD_API_PLAYLISTS_H

#include "../lib/mympd_state.h"
#include "../mpd_client/mpd_client_playlists.h"

sds mympd_api_playlist_list(struct t_partition_state *partition_state, sds buffer, long request_id,
        const long offset, const long limit, sds searchstr, enum playlist_types type);
sds mympd_api_playlist_content_list(struct t_partition_state *partition_state, sds buffer,
        long request_id, sds plist, const long offset, const long limit, sds searchstr,
        const struct t_tags *tagcols);
sds mympd_api_playlist_delete(struct t_partition_state *partition_state, sds buffer,
        long request_id, const char *playlist, bool smartpls_only);
sds mympd_api_playlist_rename(struct t_partition_state *partition_state, sds buffer,
        long request_id, const char *old_playlist, const char *new_playlist);
sds mympd_api_playlist_delete_all(struct t_partition_state *partition_state, sds buffer,
        long request_id, const char *type);
#endif
