/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2022 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include "compile_time.h"
#include "src/mympd_api/sticker.h"

#include "src/lib/jsonrpc.h"
#include "src/lib/sticker_cache.h"

/**
 * Gets the stickers from sticker cache and returns a json list
 * Shortcut for get_sticker_from_cache and print_sticker
 * @param buffer already allocated sds string to append the list
 * @param sticker_cache pointer to sticker cache
 * @param uri song uri
 * @return pointer to the modified buffer
 */
sds mympd_api_sticker_get_print(sds buffer, struct t_cache *sticker_cache, const char *uri) {
    struct t_sticker *sticker = get_sticker_from_cache(sticker_cache, uri);
    return mympd_api_sticker_print(buffer, sticker);
}

/**
 * Print the sticker struct as json list
 * @param buffer already allocated sds string to append the list
 * @param sticker pointer to sticker struct to print
 * @return pointer to the modified buffer
 */
sds mympd_api_sticker_print(sds buffer, struct t_sticker *sticker) {
    if (sticker != NULL) {
        buffer = tojson_long(buffer, "stickerPlayCount", sticker->play_count, true);
        buffer = tojson_long(buffer, "stickerSkipCount", sticker->skip_count, true);
        buffer = tojson_long(buffer, "stickerLike", sticker->like, true);
        buffer = tojson_time(buffer, "stickerLastPlayed", sticker->last_played, true);
        buffer = tojson_time(buffer, "stickerLastSkipped", sticker->last_skipped, true);
        buffer = tojson_time(buffer, "stickerElapsed", sticker->elapsed, false);
    }
    else {
        buffer = tojson_long(buffer, "stickerPlayCount", 0, true);
        buffer = tojson_long(buffer, "stickerSkipCount", 0, true);
        buffer = tojson_long(buffer, "stickerLike", 1, true);
        buffer = tojson_long(buffer, "stickerLastPlayed", 0, true);
        buffer = tojson_long(buffer, "stickerLastSkipped", 0, true);
        buffer = tojson_llong(buffer, "stickerElapsed", 0, false);
    }
    return buffer;
}
