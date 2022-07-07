/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2022 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#ifndef MYMPD_MPD_CLIENT_STICKER_H
#define MYMPD_MPD_CLIENT_STICKER_H

#include "../../dist/rax/rax.h"
#include "../lib/mympd_state.h"

struct t_sticker *get_sticker_from_cache(rax *sticker_cache, const char *uri);
sds mpd_client_sticker_list(sds buffer, rax *sticker_cache, const char *uri);
bool mpd_client_get_sticker(struct t_mpd_state *mpd_state, const char *uri, struct t_sticker *sticker);
#endif
