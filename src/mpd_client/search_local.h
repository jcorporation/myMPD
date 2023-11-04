/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2023 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#ifndef MYMPD_MPD_CLIENT_SEARCH_LOCAL_H
#define MYMPD_MPD_CLIENT_SEARCH_LOCAL_H

#include "src/lib/mympd_state.h"

enum search_filters {
    SEARCH_FILTER_ANY_TAG = -2,
    SEARCH_FILTER_MODIFIED_SINCE = -3,
    SEARCH_FILTER_ADDED_SINCE = -4
};

bool search_mpd_song(const struct mpd_song *song, sds searchstr, const struct t_tags *tags);
struct t_list *parse_search_expression_to_list(sds expression);
void *free_search_expression_list(struct t_list *expr_list);
bool search_song_expression(const struct mpd_song *song, const struct t_list *expr_list, const struct t_tags *browse_tag_types);
#endif
