/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2024 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#ifndef MYMPD_MPD_CLIENT_SEARCH_LOCAL_H
#define MYMPD_MPD_CLIENT_SEARCH_LOCAL_H

#include "dist/libmympdclient/include/mpd/client.h"
#include "src/lib/fields.h"

#include <stdbool.h>

struct t_list *parse_search_expression_to_list(const char *expression);
void *free_search_expression_list(struct t_list *expr_list);
bool search_song_expression(const struct mpd_song *song, const struct t_list *expr_list, const struct t_tags *browse_tag_types);

#endif
