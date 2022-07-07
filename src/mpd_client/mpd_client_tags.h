/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2022 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#ifndef MYMPD_MPD_CLIENT_TAGS_H
#define MYMPD_MPD_CLIENT_TAGS_H

#include "../../dist/sds/sds.h"
#include "../lib/mympd_state.h"

void mympd_mpd_song_set_last_modified(struct mpd_song *song, time_t last_modified);
bool mympd_mpd_song_add_tag_dedup(struct mpd_song *song,
		enum mpd_tag_type type, const char *value);
bool is_multivalue_tag(enum mpd_tag_type tag);
sds printAudioFormat(sds buffer, const struct mpd_audio_format *audioformat);
bool filter_mpd_song(const struct mpd_song *song, sds searchstr, const struct t_tags *tagcols);
void disable_all_mpd_tags(struct t_mpd_state *mpd_state);
void enable_all_mpd_tags(struct t_mpd_state *mpd_state);
void enable_mpd_tags(struct t_mpd_state *mpd_state, struct t_tags *enable_tags);
enum mpd_tag_type get_sort_tag(enum mpd_tag_type tag);
sds get_song_tags(sds buffer, struct t_mpd_state *mpd_state, const struct t_tags *tagcols,
                  const struct mpd_song *song);
sds get_empty_song_tags(sds buffer, struct t_mpd_state *mpd_state, const struct t_tags *tagcols,
                        const char *uri);
void check_tags(sds taglist, const char *taglistname, struct t_tags *tagtypes,
                struct t_tags *allowed_tag_types);
bool mpd_client_tag_exists(struct t_tags *tagtypes, const enum mpd_tag_type tag);
sds mpd_client_get_tag_values(struct mpd_song const *song, const enum mpd_tag_type tag, sds tag_values);
sds mpd_client_get_tag_value_string(struct mpd_song const *song, const enum mpd_tag_type tag, sds tag_values);
#endif
