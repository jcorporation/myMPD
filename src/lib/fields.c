/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2025 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

/*! \file
 * \brief Fields handling
 */

#include "compile_time.h"
#include "src/lib/fields.h"

#include <string.h>

/**
 * Copy a struct t_fields to another one
 * @param src_fields source
 * @param dst_fields destination
 */
void fields_clone(struct t_fields *src_fields, struct t_fields *dst_fields) {
    memcpy((void *)dst_fields, (void *)src_fields, sizeof(struct t_fields));
}

/**
 * (Re-)initializes a t_fields struct
 * @param fields pointer to t_fields struct
 */
void fields_reset(struct t_fields *fields) {
    mympd_mpd_tags_reset(&fields->mpd_tags);
    stickers_reset(&fields->stickers);
}

/**
 * (Re-)initializes a t_fields struct
 * @param stickers pointer to t_stickers struct
 */
void stickers_reset(struct t_stickers *stickers) {
    stickers->len = 0;
    memset(stickers->stickers, 0, sizeof(stickers->stickers));
    stickers->user_defined = false;
}

/**
 * Enables all stickers
 * @param stickers pointer to t_stickers struct
 * @param sticker_type myMPD sticker type
 */
void stickers_enable_all(struct t_stickers *stickers, enum mympd_sticker_type sticker_type) {
    switch(sticker_type) {
        case STICKER_TYPE_SONG:
            stickers->len = STICKER_COUNT;
            for (int i = 0; i < STICKER_COUNT; i++) {
                stickers->stickers[i] = i;
            }
            break;
        default:
            stickers->len = 2;
            stickers->stickers[0] = STICKER_LIKE;
            stickers->stickers[1] = STICKER_RATING;
    }
    stickers->user_defined = true;
}

/**
 * (Re-)initializes a t_mympd_mpd_tags struct
 * @param mpd_tags pointer to t_mympd_mpd_tags struct
 */
void mympd_mpd_tags_reset(struct t_mympd_mpd_tags *mpd_tags) {
    mpd_tags->len = 0;
    memset(mpd_tags->tags, 0, sizeof(mpd_tags->tags));
}

/**
 * Copy a struct t_mympd_mpd_tags to another one
 * @param src_mpd_tags source
 * @param dst_mympd_mpd_tags destination
 */
void mympd_mpd_tags_clone(struct t_mympd_mpd_tags *src_mpd_tags, struct t_mympd_mpd_tags *dst_mympd_mpd_tags) {
    memcpy((void *)dst_mympd_mpd_tags, (void *)src_mpd_tags, sizeof(struct t_mympd_mpd_tags));
}
