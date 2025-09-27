/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2025 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

/*! \file
 * \brief Jukebox library
 */

#include "compile_time.h"
#include "src/lib/jukebox.h"

#include <string.h>

/**
 * Parses the string to the jukebox mode
 * @param str string to parse
 * @return jukebox mode
 */
enum jukebox_modes jukebox_mode_parse(const char *str) {
    if (strcmp(str, "off") == 0) {
        return JUKEBOX_OFF;
    }
    if (strcmp(str, "song") == 0) {
        return JUKEBOX_ADD_SONG;
    }
    if (strcmp(str, "album") == 0) {
        return JUKEBOX_ADD_ALBUM;
    }
    if (strcmp(str, "script") == 0) {
        return JUKEBOX_SCRIPT;
    }
    return JUKEBOX_UNKNOWN;
}

/**
 * Returns the jukebox mode as string
 * @param mode the jukebox mode
 * @return jukebox mode as string
 */
const char *jukebox_mode_lookup(enum jukebox_modes mode) {
    switch (mode) {
        case JUKEBOX_OFF:
            return "off";
        case JUKEBOX_ADD_SONG:
            return "song";
        case JUKEBOX_ADD_ALBUM:
            return "album";
        case JUKEBOX_SCRIPT:
            return "script";
        case JUKEBOX_UNKNOWN:
            return NULL;
    }
    return NULL;
}
