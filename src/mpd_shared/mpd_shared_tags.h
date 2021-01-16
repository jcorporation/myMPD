/*
 SPDX-License-Identifier: GPL-2.0-or-later
 myMPD (c) 2018-2021 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#ifndef __MPD_SHARED_TAGS_H__
#define __MPD_SHARED_TAGS_H__

#include "../../dist/src/rax/rax.h"

void reset_t_tags(t_tags *tags);
void disable_all_mpd_tags(t_mpd_state *mpd_state);
void enable_all_mpd_tags(t_mpd_state *mpd_state);
void enable_mpd_tags(t_mpd_state *mpd_state, t_tags enable_tags);
enum mpd_tag_type get_sort_tag(enum mpd_tag_type tag);
sds put_song_tags(sds buffer, t_mpd_state *mpd_state, const t_tags *tagcols, const struct mpd_song *song);
sds put_empty_song_tags(sds buffer, t_mpd_state *mpd_state, const t_tags *tagcols, const char *uri);
void check_tags(sds taglist, const char *taglistname, t_tags *tagtypes, t_tags allowed_tag_types);
bool mpd_shared_tag_exists(const enum mpd_tag_type tag_types[64], const size_t tag_types_len, const enum mpd_tag_type tag);
sds mpd_shared_get_tags(struct mpd_song const *song, const enum mpd_tag_type tag, sds tags);
sds _mpd_shared_get_tags(struct mpd_song const *song, const enum mpd_tag_type tag, sds tags);
void album_cache_free(rax **album_cache);
#endif
