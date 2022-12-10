/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2022 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include "compile_time.h"
#include "src/mympd_api/sticker.h"

#include "src/lib/sticker_cache.h"

/**
 * Gets the stickers from sticker cache and returns a json list
 * Shortcut for get_sticker_from_cache and print_sticker
 * @param buffer already allocated sds string to append the list
 * @param sticker_cache pointer to sticker cache
 * @param uri song uri
 * @return pointer to the modified buffer
 */
sds mympd_api_sticker_list(sds buffer, struct t_cache *sticker_cache, const char *uri) {
    struct t_sticker *sticker = get_sticker_from_cache(sticker_cache, uri);
    return sticker_cache_print_sticker(buffer, sticker);
}
