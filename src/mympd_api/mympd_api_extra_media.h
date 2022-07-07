/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2022 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#ifndef MYMPD_API_UTILITY_H
#define MYMPD_API_UTILITY_H

#include "../lib/mympd_state.h"

sds get_extra_media(struct t_mympd_state *mympd_state, sds buffer, const char *uri, bool is_dirname);
#endif
