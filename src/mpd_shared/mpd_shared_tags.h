/*
 SPDX-License-Identifier: GPL-2.0-or-later
 myMPD (c) 2018-2020 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#ifndef __MPD_SHARED_TAGS_H__
#define __MPD_SHARED_TAGS_H__
void reset_t_tags(t_tags *tags);
void disable_all_mpd_tags(t_mpd_state *mpd_state);
void enable_all_mpd_tags(t_mpd_state *mpd_state);
void enable_mpd_tags(t_mpd_state *mpd_state, t_tags enable_tags);
enum mpd_tag_type get_sort_tag(enum mpd_tag_type tag);
char *mpd_shared_get_tag(struct mpd_song const *song, const enum mpd_tag_type tag);
sds put_song_tags(sds buffer, t_mpd_state *mpd_state, const t_tags *tagcols, const struct mpd_song *song);
sds put_empty_song_tags(sds buffer, t_mpd_state *mpd_state, const t_tags *tagcols, const char *uri);
#endif
