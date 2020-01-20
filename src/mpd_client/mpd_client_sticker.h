/*
 SPDX-License-Identifier: GPL-2.0-or-later
 myMPD (c) 2018-2020 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#ifndef __MPD_CLIENT_STICKER_H__
#define __MPD_CLIENT_STICKER_H__
bool sticker_cache_init(t_config *config, t_mpd_state *mpd_state);
void sticker_cache_free(t_mpd_state *mpd_state);
struct t_sticker *get_sticker_from_cache(t_mpd_state *mpd_state, const char *uri);

bool mpd_client_count_song_uri(t_mpd_state *mpd_state, const char *uri, const char *name, const int value);
sds mpd_client_like_song_uri(t_mpd_state *mpd_state, sds buffer, sds method, int request_id,
                             const char *uri, int value);
bool mpd_client_last_played_song_uri(t_mpd_state *mpd_state, const char *uri);
bool mpd_client_last_skipped_song_uri(t_mpd_state *mpd_state, const char *uri);
bool mpd_client_get_sticker(t_mpd_state *mpd_state, const char *uri, t_sticker *sticker);
#endif
