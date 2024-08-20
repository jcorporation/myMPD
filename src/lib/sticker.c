/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2024 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

/*! \file
 * \brief MPD sticker helpers
 */

#include "compile_time.h"
#include "src/lib/sticker.h"

#include <string.h>

/**
 * MPD sticker types names
 */
static const char *const mpd_sticker_types_names[STICKER_TYPE_COUNT] = {
    [STICKER_TYPE_SONG] = "song",
    [STICKER_TYPE_PLAYLIST] = "playlist",
    [STICKER_TYPE_FILTER] = "filter",
    [STICKER_TYPE_MYMPD_ALBUM] = "mympd_album",
    [STICKER_TYPE_TAG_TITLE] = "Title",
    [STICKER_TYPE_TAG_ALBUM] = "Album",
    [STICKER_TYPE_TAG_ARTIST] = "Artist",
    [STICKER_TYPE_TAG_ALBUM_ARTIST] = "AlbumArtist",
    [STICKER_TYPE_TAG_GENRE] = "Genre",
    [STICKER_TYPE_TAG_COMPOSER] = "Composer",
    [STICKER_TYPE_TAG_PERFORMER] = "Performer",
    [STICKER_TYPE_TAG_CONDUCTOR] = "Conductor",
    [STICKER_TYPE_TAG_WORK] = "Work",
    [STICKER_TYPE_TAG_ENSEMBLE] = "Ensemble",
    [STICKER_TYPE_TAG_LOCATION] = "Location",
    [STICKER_TYPE_TAG_LABEL] = "Label"
};

/**
 * Returns the sticker name as string
 * @param sticker_type enum mpd_sticker_type
 * @return const char* the sticker name
 */
const char *mympd_sticker_type_name_lookup(enum mympd_sticker_type sticker_type) {
    if ((unsigned)sticker_type >= STICKER_COUNT) {
        return NULL;
    }
    return mpd_sticker_types_names[sticker_type];
}

/**
 * Parses the mpd sticker type string
 * @param name sticker type string
 * @return enum mpd_tag_type the sticker enum
 */
enum mympd_sticker_type mympd_sticker_type_name_parse(const char *name) {
    if (name == NULL) {
        return STICKER_TYPE_UNKNOWN;
    }
    for (unsigned i = 0; i < STICKER_COUNT; ++i) {
        if (strcmp(name, mpd_sticker_types_names[i]) == 0) {
            return (enum mympd_sticker_type)i;
        }
    }
    return STICKER_TYPE_UNKNOWN;
}

/**
 * myMPD sticker names
 */
static const char *const mympd_sticker_names[STICKER_COUNT] = {
    [STICKER_PLAY_COUNT] = "playCount",
    [STICKER_SKIP_COUNT] = "skipCount",
    [STICKER_LIKE] = "like",
    [STICKER_LAST_PLAYED] = "lastPlayed",
    [STICKER_LAST_SKIPPED] = "lastSkipped",
    [STICKER_ELAPSED] = "elapsed",
    [STICKER_RATING] = "rating"
};

/**
 * Returns the sticker name as string
 * @param sticker enum mympd_sticker_types
 * @return const char* the sticker name
 */
const char *sticker_name_lookup(enum mympd_sticker_names sticker) {
    if ((unsigned)sticker >= STICKER_COUNT) {
        return NULL;
    }
    return mympd_sticker_names[sticker];
}

/**
 * Parses the sticker name
 * @param name sticker name
 * @return enum mpd_tag_type the sticker enum
 */
enum mympd_sticker_names sticker_name_parse(const char *name) {
    if (name == NULL) {
        return STICKER_UNKNOWN;
    }
    for (unsigned i = 0; i < STICKER_COUNT; ++i) {
        if (strcmp(name, mympd_sticker_names[i]) == 0) {
            return (enum mympd_sticker_names)i;
        }
    }
    return STICKER_UNKNOWN;
}

/**
 * Initializes the sticker struct
 * @param sticker struct to init
 */
void sticker_struct_init(struct t_sticker *sticker) {
    memset(sticker->mympd, 0, sizeof(sticker->mympd));
    sticker->mympd[STICKER_LIKE] = 1;
    list_init(&sticker->user);
}

/**
 * Clears the sticker struct
 * @param sticker 
 */
void sticker_struct_clear(struct t_sticker *sticker) {
    memset(sticker->mympd, 0, sizeof(sticker->mympd));
    list_clear(&sticker->user);
}

/**
 * Converts a string to a mpd sticker operator
 * @param str string to parse
 * @return mpd sticker operator
 */
enum mpd_sticker_operator sticker_oper_parse(const char *str) {
    if (str[0] == '=') { return MPD_STICKER_OP_EQ; }
    if (str[0] == '>') { return MPD_STICKER_OP_GT; }
    if (str[0] == '<') { return MPD_STICKER_OP_LT; }
    if (strcmp(str, "eq") == 0) { return MPD_STICKER_OP_EQ_INT; }
    if (strcmp(str, "gt") == 0) { return MPD_STICKER_OP_GT_INT; }
    if (strcmp(str, "lt") == 0) { return MPD_STICKER_OP_LT_INT; }
    return MPD_STICKER_OP_UNKOWN;
}

/**
 * Converts a string to a mpd sticker operator
 * @param str string to parse
 * @return mpd sticker sort type
 */
enum mpd_sticker_sort sticker_sort_parse(const char *str) {
    if (strcmp(str, "uri") == 0) { return MPD_STICKER_SORT_URI; }
    if (strcmp(str, "value") == 0) { return MPD_STICKER_SORT_VALUE; }
    if (strcmp(str, "value_int") == 0) { return MPD_STICKER_SORT_VALUE_INT; }
    return MPD_STICKER_SORT_UNKOWN;
}
