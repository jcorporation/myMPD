/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2024 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#ifndef MYMPD_CACHE_RAX_ALBUM_H
#define MYMPD_CACHE_RAX_ALBUM_H

#include "dist/sds/sds.h"
#include "src/lib/cache_rax.h"
#include "src/lib/config_def.h"
#include "src/lib/fields.h"

#include <stdbool.h>

enum album_modes parse_album_mode(const char *mode_str);
const char *lookup_album_mode(enum album_modes mode);

bool album_cache_remove(sds workdir);
bool album_cache_read(struct t_cache *album_cache, sds workdir, const struct t_albums_config *album_config);
bool album_cache_write(struct t_cache *album_cache, sds workdir, const struct t_mpd_tags *album_tags, const struct t_albums_config *album_config, bool free_data);

sds album_cache_get_key(sds albumkey, const struct mpd_song *song, const struct t_albums_config *album_config);
struct mpd_song *album_cache_get_album(struct t_cache *album_cache, sds key);
void album_cache_free(struct t_cache *album_cache);
void album_cache_free_rt(rax *album_cache_rt);

unsigned album_get_discs(const struct mpd_song *album);
unsigned album_get_total_time(const struct mpd_song *album);
unsigned album_get_song_count(const struct mpd_song *album);

void album_cache_set_discs(struct mpd_song *album, const struct mpd_song *song);
void album_cache_set_disc_count(struct mpd_song *album, unsigned count);
void album_cache_set_last_modified(struct mpd_song *album, const struct mpd_song *song);
void album_cache_set_added(struct mpd_song *album, const struct mpd_song *song);
void album_cache_set_total_time(struct mpd_song *album, unsigned duration);
void album_cache_inc_total_time(struct mpd_song *album, const struct mpd_song *song);
void album_cache_set_song_count(struct mpd_song *album, unsigned count);
void album_cache_inc_song_count(struct mpd_song *album);
bool album_cache_append_tags(struct mpd_song *album, const struct mpd_song *song, const struct t_mpd_tags *tags);
bool album_cache_copy_tags(struct mpd_song *song, enum mpd_tag_type src, enum mpd_tag_type dst);
void album_cache_set_uri(struct mpd_song *album, const char *uri);

#endif
