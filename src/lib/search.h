/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2025 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

/*! \file
 * \brief Search implementation
 */

#ifndef MYMPD_LIB_SEARCH_H
#define MYMPD_LIB_SEARCH_H

#include "dist/libmympdclient/include/mpd/client.h"
#include "src/lib/fields.h"
#include "src/lib/webradio.h"

#include <stdbool.h>

/**
 * Types of struct to search
 */
enum search_type {
    SEARCH_TYPE_SONG,
    SEARCH_TYPE_WEBRADIO
};

struct t_list *parse_search_expression_to_list(const char *expression, enum search_type type);
void *free_search_expression_list(struct t_list *expr_list);
bool search_expression_song(const struct mpd_song *song, const struct t_list *expr_list, const struct t_mpd_tags *any_tag_types);
bool search_expression_webradio(const struct t_webradio_data *webradio, const struct t_list *expr_list, const struct t_webradio_tags *any_tag_types);

#endif
