/*
 SPDX-License-Identifier: GPL-2.0-or-later
 myMPD (c) 2018-2020 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#ifndef __MPD_CLIENT_STICKER_H__
#define __MPD_CLIENT_STICKER_H__
bool mpd_client_sticker_inc_play_count(t_mpd_client_state *mpd_client_state, const char *uri);
bool mpd_client_sticker_inc_skip_count(t_mpd_client_state *mpd_client_state, const char *uri);
bool mpd_client_sticker_like(t_mpd_client_state *mpd_client_state, const char *uri, int value);
bool mpd_client_sticker_last_played(t_mpd_client_state *mpd_client_state, const char *uri);
bool mpd_client_sticker_last_skipped(t_mpd_client_state *mpd_client_state, const char *uri);

bool mpd_client_sticker_dequeue(t_mpd_client_state *mpd_client_state);

bool sticker_cache_init(t_config *config, t_mpd_client_state *mpd_client_state);
struct t_sticker *get_sticker_from_cache(t_mpd_client_state *mpd_client_state, const char *uri);
bool mpd_client_get_sticker(t_mpd_client_state *mpd_client_state, const char *uri, t_sticker *sticker);
#endif
