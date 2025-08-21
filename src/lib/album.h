/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2025 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

/*! \file
 * \brief Album implementation
 */

#ifndef MYMPD_ALBUM_H
#define MYMPD_ALBUM_H

#include "src/lib/fields.h"

#include <stdbool.h>

struct mpd_song *album_new(void);
struct mpd_song *album_new_uri(const char *uri);

const char *album_get_uri(const struct mpd_song *album);
unsigned album_get_duration(const struct mpd_song *album);
time_t album_get_last_modified(const struct mpd_song *album);
time_t album_get_added(const struct mpd_song *album);
unsigned album_get_discs(const struct mpd_song *album);
unsigned album_get_total_time(const struct mpd_song *album);
unsigned album_get_song_count(const struct mpd_song *album);
const char *album_get_tag(const struct mpd_song *album, enum mpd_tag_type type, unsigned idx);

void album_set_discs(struct mpd_song *album, const char *disc);
void album_set_disc_count(struct mpd_song *album, unsigned count);
void album_set_last_modified(struct mpd_song *album, time_t last_modified);
void album_set_added(struct mpd_song *album, time_t added);
void album_set_total_time(struct mpd_song *album, unsigned duration);
void album_inc_total_time(struct mpd_song *album, unsigned duration);
void album_set_song_count(struct mpd_song *album, unsigned count);
void album_inc_song_count(struct mpd_song *album);
bool album_append_tag(struct mpd_song *song, enum mpd_tag_type type, const char *value);
bool album_append_tags(struct mpd_song *album, const struct mpd_song *song, const struct t_mympd_mpd_tags *tags);
bool album_copy_tags(struct mpd_song *song, enum mpd_tag_type src, enum mpd_tag_type dst);
void album_set_uri(struct mpd_song *album, const char *uri);

#endif
