/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2024 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include "compile_time.h"
#include "src/lib/fields.h"

#include <string.h>

/**
 * Copy a struct t_fields to another one
 * @param src_tag_list source
 * @param dst_tag_list destination
 */
void fields_clone(struct t_fields *src_fields, struct t_fields *dst_fields) {
    memcpy((void *)dst_fields, (void *)src_fields, sizeof(struct t_fields));
}

/**
 * (Re-)initializes a t_fields struct
 * @param fields pointer to t_fields struct
 */
void fields_reset(struct t_fields *fields) {
    tags_reset(&fields->tags);
    stickers_reset(&fields->stickers);
}

/**
 * (Re-)initializes a t_fields struct
 * @param fields pointer to t_fields struct
 */
void stickers_reset(struct t_stickers *stickers) {
    stickers->len = 0;
    memset(stickers->stickers, 0, sizeof(stickers->stickers));
}

/**
 * Enables all stickers
 * @param fields pointer to t_fields struct
 */
void stickers_enable_all(struct t_stickers *stickers) {
    stickers->len = STICKER_COUNT;
    for (int i = 0; i < STICKER_COUNT; i++) {
        stickers->stickers[i] = i;
    }
}

/**
 * (Re-)initializes a t_tags struct
 * @param fields pointer to t_fields struct
 */
void tags_reset(struct t_tags *tags) {
    tags->len = 0;
    memset(tags->tags, 0, sizeof(tags->tags));
}

/**
 * Copy a struct t_fields to another one
 * @param src_tag_list source
 * @param dst_tag_list destination
 */
void tags_clone(struct t_tags *src_fields, struct t_tags *dst_fields) {
    memcpy((void *)dst_fields, (void *)src_fields, sizeof(struct t_tags));
}
