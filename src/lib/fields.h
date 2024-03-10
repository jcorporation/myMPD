/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2024 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#ifndef MYMPD_FIELDS_H
#define MYMPD_FIELDS_H

#include "dist/libmympdclient/include/mpd/client.h"
#include "src/lib/sticker.h"

/**
 * Struct for a mpd tag and sticker lists
 * libmpdclient uses a similar declaration, but for tags only
 */
struct t_tags {
    size_t len;                  //!< number of tags in the array
    enum mpd_tag_type tags[64];  //!< tags array
};

/**
 * Struct for a mpd tag and sticker lists
 * libmpdclient uses a similar declaration, but for tags only
 */
struct t_stickers {
    size_t len;                                        //!< number of stickers in the array
    enum mympd_sticker_types stickers[STICKER_COUNT];  //!< stickers array
};

/**
 * Struct for a mpd tag and sticker lists
 * libmpdclient uses a similar declaration, but for tags only
 */
struct t_fields {
    struct t_tags tags;          //!< tags
    struct t_stickers stickers;  //!< stickers
};

enum sort_by_type {
    SORT_BY_TAG,
    SORT_BY_LAST_MODIFIED,
    SORT_BY_ADDED,
    SORT_BY_FILENAME
};

void fields_clone(struct t_fields *src_fields, struct t_fields *dst_fields);
void fields_reset(struct t_fields *fields);

void stickers_reset(struct t_stickers *stickers);
void stickers_enable_all(struct t_stickers *stickers);

void tags_clone(struct t_tags *src_tags, struct t_tags *dst_tags);
void tags_reset(struct t_tags *tags);

#endif
