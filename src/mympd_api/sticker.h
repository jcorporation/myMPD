/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2023 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#ifndef MYMPD_API_STICKER_H
#define MYMPD_API_STICKER_H

#include "src/lib/mympd_state.h"
#include "src/lib/sticker_cache.h"

bool mympd_api_sticker_set_like(struct t_partition_state *partition_state, sds uri, int like, sds *error);
sds mympd_api_sticker_get_print(sds buffer, struct t_cache *sticker_cache, const char *uri);
sds mympd_api_sticker_print(sds buffer, struct t_sticker *sticker);

#endif
