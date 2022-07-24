/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2022 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#ifndef MYMPD_API_COVER_H
#define MYMPD_API_COVER_H

#include "../lib/mympd_state.h"

sds mympd_api_albumart_getcover(struct t_mpd_state *mpd_state, sds buffer, long request_id,
        const char *uri, sds *binary);
#endif
