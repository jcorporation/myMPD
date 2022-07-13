/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2022 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#ifndef MYMPD_STICKER_CACHE_H
#define MYMPD_STICKER_CACHE_H

#include "../../dist/rax/rax.h"

struct t_sticker *get_sticker_from_cache(rax *sticker_cache, const char *uri);

#endif
