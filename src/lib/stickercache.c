/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2022 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include "mympd_config_defs.h"
#include "stickercache.h"

#include "log.h"
#include "utility.h"

#include <string.h>

/** Gets the sticker struct from sticker cache
 * @param sticker_cache pointer to sticker cache
 * @param uri song uri
 * @return pointer to the sticker struct
 */
struct t_sticker *get_sticker_from_cache(rax *sticker_cache, const char *uri) {
    //ignore stream uris
    if (is_streamuri(uri) == true) {
        return NULL;
    }
    //check for uninitialized sticker cache
    if (sticker_cache == NULL) {
        return NULL;
    }
    //try to get sticker
    void *data = raxFind(sticker_cache, (unsigned char*)uri, strlen(uri));
    if (data == raxNotFound) {
        MYMPD_LOG_ERROR("Sticker for uri \"%s\" not found in cache", uri);
        return NULL;
    }
    return (struct t_sticker *) data;
}
