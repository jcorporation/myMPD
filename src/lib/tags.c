/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2024 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include "compile_time.h"
#include "src/lib/tags.h"

#include <string.h>

/**
 * Copy a struct t_tags struct to another one
 * @param src_tag_list source
 * @param dst_tag_list destination
 */
void copy_tag_types(struct t_tags *src_tag_list, struct t_tags *dst_tag_list) {
    memcpy((void *)dst_tag_list, (void *)src_tag_list, sizeof(struct t_tags));
}

/**
 * (Re-)initializes a t_tags struct
 * @param tags pointer to t_tags struct
*/
void reset_t_tags(struct t_tags *tags) {
    tags->tags_len = 0;
    memset(tags->tags, 0, sizeof(tags->tags));
    tags->stickers_len = 0;
    memset(tags->stickers, 0, sizeof(tags->stickers));
}

void tags_enable_all_stickers(struct t_tags *tags) {
    tags->stickers_len = STICKER_COUNT;
    for (int i = 0; i < STICKER_COUNT; i++) {
        tags->stickers[i] = i;
    }
}
