/*
 SPDX-License-Identifier: GPL-2.0-or-later
 myMPD (c) 2018-2021 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#ifndef __MPD_CLIENT_STICKER_H__
#define __MPD_CLIENT_STICKER_H__
bool mpd_client_sticker_inc_play_count(struct t_mympd_state *mympd_state, const char *uri);
bool mpd_client_sticker_inc_skip_count(struct t_mympd_state *mympd_state, const char *uri);
bool mpd_client_sticker_like(struct t_mympd_state *mympd_state, const char *uri, int value);
bool mpd_client_sticker_last_played(struct t_mympd_state *mympd_state, const char *uri);
bool mpd_client_sticker_last_skipped(struct t_mympd_state *mympd_state, const char *uri);
bool mpd_client_sticker_dequeue(struct t_mympd_state *mympd_state);
#endif
