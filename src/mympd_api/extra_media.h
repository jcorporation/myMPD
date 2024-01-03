/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2024 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#ifndef MYMPD_API_EXTRA_MEDIA_H
#define MYMPD_API_EXTRA_MEDIA_H

#include "src/lib/mympd_state.h"

sds mympd_api_get_extra_media(sds buffer, struct t_mpd_state *mpd_state, sds booklet_name, sds info_txt_name, const char *uri, bool is_dirname);

#endif
