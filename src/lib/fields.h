/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2024 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

/*! \file
 * \brief Fields handling
 */

#ifndef MYMPD_FIELDS_H
#define MYMPD_FIELDS_H

#include "dist/libmympdclient/include/mpd/client.h"
#include "src/lib/sticker.h"

/**
 * Struct for a mpd tag list
 * libmpdclient uses a similar declaration
 */
struct t_mpd_tags {
    size_t len;                  //!< number of tags in the array
    enum mpd_tag_type tags[64];  //!< tags array
};

/**
 * Struct for a mpd tag and sticker lists
 * libmpdclient uses a similar declaration, but for tags only
 */
struct t_stickers {
    size_t len;                                        //!< number of stickers in the array
    enum mympd_sticker_names stickers[STICKER_COUNT];  //!< stickers array
    bool user_defined;                                 //!< user defines stickers
};

/**
 * Struct for a mpd tag and sticker lists
 * libmpdclient uses a similar declaration, but for tags only
 */
struct t_fields {
    struct t_mpd_tags mpd_tags;  //!< mpd tags
    struct t_stickers stickers;  //!< stickers
};

/**
 * Sort type enums
 */
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

void mpd_tags_clone(struct t_mpd_tags *src_mpd_tags, struct t_mpd_tags *dst_mpd_tags);
void mpd_tags_reset(struct t_mpd_tags *mpd_tags);

#endif
