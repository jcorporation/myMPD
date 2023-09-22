/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2023 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#ifndef MYMPD_STICKER_H
#define MYMPD_STICKER_H

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
    STICKER_COUNT
};

const char *sticker_name_lookup(enum mympd_sticker_types sticker);
enum mympd_sticker_types sticker_name_parse(const char *name);

#endif
