/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2024 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

/*! \file
 * \brief MPD sticker helpers
 */

#ifndef MYMPD_STICKER_H
#define MYMPD_STICKER_H

#include "dist/libmympdclient/include/mpd/client.h"
#include "src/lib/list.h"

/**
 * MPD sticker types
 */
enum mympd_sticker_type {
    STICKER_TYPE_UNKNOWN = -1,
    STICKER_TYPE_SONG,
    STICKER_TYPE_PLAYLIST,
    STICKER_TYPE_FILTER,
    STICKER_TYPE_MYMPD_ALBUM,
    STICKER_TYPE_TAG_TITLE,
    STICKER_TYPE_TAG_ALBUM,
    STICKER_TYPE_TAG_ARTIST,
    STICKER_TYPE_TAG_ALBUM_ARTIST,
    STICKER_TYPE_TAG_GENRE,
    STICKER_TYPE_TAG_COMPOSER,
    STICKER_TYPE_TAG_PERFORMER,
    STICKER_TYPE_TAG_CONDUCTOR,
    STICKER_TYPE_TAG_WORK,
    STICKER_TYPE_TAG_ENSEMBLE,
    STICKER_TYPE_TAG_LOCATION,
    STICKER_TYPE_TAG_LABEL,
    STICKER_TYPE_COUNT
};

/**
 * Valid values for like sticker
 */
enum sticker_like {
    STICKER_LIKE_HATE = 0,
    STICKER_LIKE_NEUTRAL = 1,
    STICKER_LIKE_LOVE = 2
};

/**
 * myMPD sticker types
 */
enum mympd_sticker_names {
    STICKER_UNKNOWN = -1,
    STICKER_PLAY_COUNT,
    STICKER_SKIP_COUNT,
    STICKER_LIKE,
    STICKER_LAST_PLAYED,
    STICKER_LAST_SKIPPED,
    STICKER_ELAPSED,
    STICKER_RATING,
    STICKER_COUNT
};

/**
 * myMPD feedback types
 */
enum mympd_feedback_type {
    FEEDBACK_LIKE,
    FEEDBACK_STAR
};

/**
 * MPD sticker values
 */
struct t_sticker {
    int64_t mympd[STICKER_COUNT];  //!< array of myMPD stickers
    struct t_list user;            //!< list of user defined stickers
};

const char *mympd_sticker_type_name_lookup(enum mympd_sticker_type sticker_type);
enum mympd_sticker_type mympd_sticker_type_name_parse(const char *name);

const char *sticker_name_lookup(enum mympd_sticker_names sticker);
enum mympd_sticker_names sticker_name_parse(const char *name);
void sticker_struct_init(struct t_sticker *sticker);
void sticker_struct_clear(struct t_sticker *sticker);

enum mpd_sticker_operator sticker_oper_parse(const char *str);
enum mpd_sticker_sort sticker_sort_parse(const char *str);

#endif
