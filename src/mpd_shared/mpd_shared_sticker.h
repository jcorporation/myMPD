/*
 SPDX-License-Identifier: GPL-2.0-or-later
 myMPD (c) 2018-2020 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#ifndef __MPD_SHARED_STICKER_H__
#define __MPD_SHARED_STICKER_H__
bool mpd_shared_get_sticker(t_mpd_state *mpd_state, const char *uri, t_sticker *sticker);
void sticker_cache_free(rax *rdx);
#endif
