/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2023 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#ifndef MYMPD_API_UTILITY_H
#define MYMPD_API_UTILITY_H

#include "src/lib/mympd_state.h"

sds mympd_api_get_extra_media(struct t_mpd_state *mpd_state, sds buffer, const char *uri, bool is_dirname);
#endif
