/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2022 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#ifndef MYMPD_ALBUM_CACHE_H
#define MYMPD_ALBUM_CACHE_H

#include "../../dist/rax/rax.h"
#include "../../dist/sds/sds.h"
#include "../lib/mympd_state.h"

#include <stdbool.h>

sds album_cache_get_key(struct mpd_song *song, sds albumkey, enum mpd_tag_type tag_albumartist);
struct mpd_song *album_cache_get_album(rax *album_cache, sds key);

unsigned album_get_discs(struct mpd_song *album);
unsigned album_get_total_time(struct mpd_song *album);
unsigned album_get_song_count(struct mpd_song *album);

void album_cache_set_discs(struct mpd_song *album, struct mpd_song *song);
void album_cache_set_last_modified(struct mpd_song *album, struct mpd_song *song);
void album_cache_inc_total_time(struct mpd_song *album, struct mpd_song *song);
void album_cache_set_song_count(struct mpd_song *album, unsigned count);
void album_cache_inc_song_count(struct mpd_song *album);
bool album_cache_append_tags(struct mpd_song *album,
		struct mpd_song *song, struct t_tags *tags);

#endif
