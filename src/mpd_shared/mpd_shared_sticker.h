/*
 SPDX-License-Identifier: GPL-2.0-or-later
 myMPD (c) 2018-2021 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#ifndef __MPD_SHARED_STICKER_H__
#define __MPD_SHARED_STICKER_H__

#include "../../dist/src/rax/rax.h"
#include "../mympd_state.h"

struct t_sticker *get_sticker_from_cache(rax *sticker_cache, const char *uri);
sds mpd_shared_sticker_list(sds buffer, rax *sticker_cache, const char *uri);
bool mpd_shared_get_sticker(struct t_mpd_state *mpd_state, const char *uri, struct t_sticker *sticker);
void sticker_cache_free(rax **sticker_cache);
#endif
