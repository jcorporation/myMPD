/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2022 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#ifndef MYMPD_API_STICKER_H
#define MYMPD_API_STICKER_H

#include "src/lib/mympd_state.h"
#include "src/lib/sticker_cache.h"

sds mympd_api_sticker_list(sds buffer, struct t_cache *sticker_cache, const char *uri);
sds mympd_api_print_sticker(sds buffer, struct t_sticker *sticker);

#endif
