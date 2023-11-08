/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2023 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include "compile_time.h"
#include "src/lib/sticker.h"

#include <string.h>

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
const char *sticker_name_lookup(enum mympd_sticker_types sticker) {
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
enum mympd_sticker_types sticker_name_parse(const char *name) {
    if (name == NULL) {
        return STICKER_UNKNOWN;
    }
    for (unsigned i = 0; i < STICKER_COUNT; ++i) {
        if (strcmp(name, mympd_sticker_names[i]) == 0) {
            return (enum mympd_sticker_types)i;
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
