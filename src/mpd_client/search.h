/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2023 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#ifndef MYMPD_MPD_CLIENT_SEARCH_H
#define MYMPD_MPD_CLIENT_SEARCH_H

#include "src/lib/mympd_state.h"

bool mpd_client_search_add_to_plist(struct t_partition_state *partition_state, const char *expression,
        const char *plist, unsigned to, const char *sort, bool sortdesc, sds *error);
bool mpd_client_search_add_to_queue(struct t_partition_state *partition_state, const char *expression,
        unsigned to, enum mpd_position_whence whence, const char *sort, bool sortdesc, sds *error);

bool mpd_client_add_search_sort_param(struct t_partition_state *partition_state, const char *sort, bool sortdesc, bool check_version);
sds get_search_expression_album(enum mpd_tag_type tag_albumartist, struct mpd_song *album,
        const struct t_albums_config *album_config);
sds get_search_expression_album_disc(enum mpd_tag_type tag_albumartist, struct mpd_song *album,
        const char *disc, const struct t_albums_config *album_config);
sds escape_mpd_search_expression(sds buffer, const char *tag, const char *operator, const char *value);
#endif
