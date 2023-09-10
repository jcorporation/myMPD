/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2023 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#ifndef MYMPD_ALBUM_CACHE_H
#define MYMPD_ALBUM_CACHE_H

#include "dist/sds/sds.h"
#include "src/lib/mympd_state.h"

#include <stdbool.h>

bool album_cache_remove(sds workdir);
bool album_cache_read(struct t_cache *album_cache, sds workdir);
bool album_cache_write(struct t_cache *album_cache, sds workdir, const struct t_tags *album_tags, bool free_data);

sds album_cache_get_key(sds albumkey, const struct mpd_song *song);
struct mpd_song *album_cache_get_album(struct t_cache *album_cache, sds key);
void album_cache_free(struct t_cache *album_cache);

unsigned album_get_discs(const struct mpd_song *album);
unsigned album_get_total_time(const struct mpd_song *album);
unsigned album_get_song_count(const struct mpd_song *album);

void album_cache_set_discs(struct mpd_song *album, const struct mpd_song *song);
void album_cache_set_disc_count(struct mpd_song *album, unsigned count);
void album_cache_set_last_modified(struct mpd_song *album, const struct mpd_song *song);
void album_cache_inc_total_time(struct mpd_song *album, const struct mpd_song *song);
void album_cache_set_song_count(struct mpd_song *album, unsigned count);
void album_cache_inc_song_count(struct mpd_song *album);
bool album_cache_append_tags(struct mpd_song *album, const struct mpd_song *song, const struct t_tags *tags);
bool album_cache_copy_tags(struct mpd_song *song, enum mpd_tag_type src, enum mpd_tag_type dst);

#endif
