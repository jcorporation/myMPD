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

#include <time.h>

/**
 * MPD sticker types
 */
enum mpd_sticker_type {
    MPD_STICKER_TYPE_UNKNOWN = -1,
    MPD_STICKER_TYPE_SONG,
    MPD_STICKER_TYPE_PLAYLIST,
    MPD_STICKER_TYPE_FILTER,
    MPD_STICKER_TYPE_TAG_TITLE,
    MPD_STICKER_TYPE_TAG_ALBUM,
    MPD_STICKER_TYPE_TAG_ARTIST,
    MPD_STICKER_TYPE_TAG_ALBUM_ARTIST,
    MPD_STICKER_TYPE_TAG_GENRE,
    MPD_STICKER_TYPE_TAG_COMPOSER,
    MPD_STICKER_TYPE_TAG_PERFORMER,
    MPD_STICKER_TYPE_TAG_CONDUCTOR,
    MPD_STICKER_TYPE_TAG_WORK,
    MPD_STICKER_TYPE_TAG_ENSEMBLE,
    MPD_STICKER_TYPE_TAG_LOCATION,
    MPD_STICKER_TYPE_TAG_LABEL,
    MPD_STICKER_TYPE_COUNT
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
enum mympd_sticker_types {
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

const char *mpd_sticker_type_name_lookup(enum mpd_sticker_type sticker_type);
enum mpd_sticker_type mpd_sticker_type_name_parse(const char *name);

const char *sticker_name_lookup(enum mympd_sticker_types sticker);
enum mympd_sticker_types sticker_name_parse(const char *name);
void sticker_struct_init(struct t_sticker *sticker);
void sticker_struct_clear(struct t_sticker *sticker);

enum mpd_sticker_operator sticker_oper_parse(const char *str);
enum mpd_sticker_sort sticker_sort_parse(const char *str);

#endif
