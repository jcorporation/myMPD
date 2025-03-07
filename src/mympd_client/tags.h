/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2025 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

/*! \file
 * \brief MPD tags helper functions
 */

#ifndef MYMPD_MPD_CLIENT_TAGS_H
#define MYMPD_MPD_CLIENT_TAGS_H

#include "dist/sds/sds.h"
#include "src/lib/mympd_state.h"

time_t mympd_client_get_db_mtime(struct t_partition_state *partition_state);
bool mympd_mpd_song_add_tag_dedup(struct mpd_song *song,
        enum mpd_tag_type type, const char *value);
bool is_multivalue_tag(enum mpd_tag_type tag);
bool is_numeric_tag(enum mpd_tag_type tag);
sds printAudioFormat(sds buffer, const struct mpd_audio_format *audioformat);
bool disable_all_mpd_tags(struct t_partition_state *partition_state);
bool enable_all_mpd_tags(struct t_partition_state *partition_state);
bool enable_mpd_tags(struct t_partition_state *partition_state, const struct t_mympd_mpd_tags *enable_tags);
enum mpd_tag_type get_sort_tag(enum mpd_tag_type tag, const struct t_mympd_mpd_tags *available_tags);
sds print_song_tags(sds buffer, const struct t_mpd_state *mpd_state, const struct t_mympd_mpd_tags *tagcols,
        const struct mpd_song *song);
sds print_album_tags(sds buffer, const struct t_mpd_state *mpd_state, const struct t_mympd_mpd_tags *tagcols,
        const struct mpd_song *album);
void check_tags(sds taglist, const char *taglistname, struct t_mympd_mpd_tags *tagtypes,
        const struct t_mympd_mpd_tags *allowed_tag_types);
bool mympd_client_tag_exists(const struct t_mympd_mpd_tags *tagtypes, enum mpd_tag_type tag);
sds mympd_client_get_tag_values(const struct mpd_song *song, enum mpd_tag_type tag, sds tag_values);
sds mympd_client_get_tag_value_string(const struct mpd_song *song, enum mpd_tag_type tag, sds tag_values);
sds print_tags_array(sds buffer, const char *tagsname, const struct t_mympd_mpd_tags *tags);
sds mympd_client_get_tag_value_padded(const struct mpd_song *song, enum mpd_tag_type tag, const char pad, size_t len, sds tag_values);
int mympd_client_get_tag_value_int(const struct mpd_song *song, enum mpd_tag_type tag);
sds get_sort_key(sds key, enum sort_by_type sort_by, enum mpd_tag_type sort_tag, const struct mpd_song *song);

#endif
