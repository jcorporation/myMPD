/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2022 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#ifndef MYMPD_MPD_CLIENT_SEARCH_LOCAL_H
#define MYMPD_MPD_CLIENT_SEARCH_LOCAL_H

#include "../lib/mympd_state.h"

bool search_mpd_song(const struct mpd_song *song, sds searchstr, const struct t_tags *tags);
struct t_list *parse_search_expression_to_list(sds expression);
void clear_search_expression_list(struct t_list *expr_list);
bool search_song_expression(struct mpd_song *song, struct t_list *expr_list, struct t_tags *browse_tag_types);
#endif
