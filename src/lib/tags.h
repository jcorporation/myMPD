/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2023 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#ifndef MYMPD_TAGS_H
#define MYMPD_TAGS_H

#include "dist/libmympdclient/include/mpd/client.h"
#include "src/lib/sticker.h"

/**
 * Struct for a mpd tag and sticker lists
 * libmpdclient uses a similar declaration, but for tags only
 */
struct t_tags {
    size_t tags_len;                                   //!< number of tags in the array
    enum mpd_tag_type tags[64];                        //!< tags array
    size_t stickers_len;                               //!< number of stickers in the array
    enum mympd_sticker_types stickers[STICKER_COUNT];  //!< stickers array
};

enum sort_by_type {
    SORT_BY_TAG,
    SORT_BY_LAST_MODIFIED,
    SORT_BY_ADDED,
    SORT_BY_FILENAME
};

void copy_tag_types(struct t_tags *src_tag_list, struct t_tags *dst_tag_list);
void reset_t_tags(struct t_tags *tags);
void tags_enable_all_stickers(struct t_tags *tags);

#endif
