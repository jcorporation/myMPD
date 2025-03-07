/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2025 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

/*! \file
 * \brief Helper functions for MPD search
 */

#ifndef MYMPD_MPD_CLIENT_SEARCH_H
#define MYMPD_MPD_CLIENT_SEARCH_H

#include "src/lib/mympd_state.h"

bool mympd_client_search_add_to_plist(struct t_partition_state *partition_state, const char *expression,
        const char *plist, unsigned to, const char *sort, bool sortdesc, sds *error);
bool mympd_client_search_add_to_plist_window(struct t_partition_state *partition_state, const char *expression,
        const char *plist, unsigned to, const char *sort, bool sortdesc, unsigned start, unsigned end, sds *error);
bool mympd_client_search_add_to_queue(struct t_partition_state *partition_state, const char *expression,
        unsigned to, enum mpd_position_whence whence, const char *sort, bool sortdesc, sds *error);
bool mympd_client_search_add_to_queue_window(struct t_partition_state *partition_state, const char *expression,
        unsigned to, enum mpd_position_whence whence, const char *sort, bool sortdesc,
        unsigned start, unsigned end, sds *error);

bool mympd_client_add_search_sort_param(struct t_partition_state *partition_state, const char *sort, bool sortdesc, bool check_version);
bool mympd_client_add_search_group_param(struct mpd_connection *conn, enum mpd_tag_type tag);
sds get_search_expression_album(sds buffer, enum mpd_tag_type tag_albumartist, struct mpd_song *album,
        const struct t_albums_config *album_config);
sds get_search_expression_album_tag(sds buffer, enum mpd_tag_type tag_albumartist, struct mpd_song *album,
        enum mpd_tag_type tag, const char *tag_value, const struct t_albums_config *album_config);
sds escape_mpd_search_expression(sds buffer, const char *tag, const char *operator, const char *value);

#endif
