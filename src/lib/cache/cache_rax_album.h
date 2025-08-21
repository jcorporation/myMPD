/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2025 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

/*! \file
 * \brief Album cache
 */

#ifndef MYMPD_CACHE_RAX_ALBUM_H
#define MYMPD_CACHE_RAX_ALBUM_H

#include "dist/sds/sds.h"
#include "src/lib/cache/cache_rax.h"
#include "src/lib/config_def.h"
#include "src/lib/fields.h"

#include <stdbool.h>

enum album_modes parse_album_mode(const char *mode_str);
const char *lookup_album_mode(enum album_modes mode);

bool album_cache_remove(sds workdir);
bool album_cache_read(struct t_cache *album_cache, sds workdir, const struct t_albums_config *album_config);
bool album_cache_write(struct t_cache *album_cache, sds workdir, const struct t_mympd_mpd_tags *album_tags, const struct t_albums_config *album_config, bool free_data);

sds album_cache_get_key(sds albumkey, const struct mpd_song *song, const struct t_albums_config *album_config);
struct mpd_song *album_cache_get_album(struct t_cache *album_cache, sds key);
void album_cache_free(struct t_cache *album_cache);
void album_cache_free_rt(rax *album_cache_rt);
void album_cache_free_rt_void(void *album_cache_rt);

#endif
