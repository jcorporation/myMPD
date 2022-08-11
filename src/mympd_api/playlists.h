/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2022 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#ifndef MYMPD_API_PLAYLISTS_H
#define MYMPD_API_PLAYLISTS_H

#include "../lib/mympd_state.h"
#include "../mpd_client/playlists.h"

enum plist_delete_criterias {
    PLAYLIST_DELETE_UNKNOWN = -1,
    PLAYLIST_DELETE_EMPTY,
    PLAYLIST_DELETE_SMARTPLS,
    PLAYLIST_DELETE_ALL
};

enum plist_delete_criterias parse_plist_delete_criteria(const char *str);

sds mympd_api_playlist_list(struct t_partition_state *partition_state, sds buffer, long request_id,
        long offset, long limit, sds searchstr, enum playlist_types type);
sds mympd_api_playlist_content_list(struct t_partition_state *partition_state, sds buffer,
        long request_id, sds plist, long offset, long limit, sds searchstr,
        const struct t_tags *tagcols);
sds mympd_api_playlist_delete(struct t_partition_state *partition_state, sds buffer,
        long request_id, const char *playlist, bool smartpls_only);
sds mympd_api_playlist_rename(struct t_partition_state *partition_state, sds buffer,
        long request_id, const char *old_playlist, const char *new_playlist);
sds mympd_api_playlist_delete_all(struct t_partition_state *partition_state, sds buffer,
        long request_id, enum plist_delete_criterias criteria);
#endif
