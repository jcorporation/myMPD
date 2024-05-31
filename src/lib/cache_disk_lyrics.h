/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2024 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#ifndef MYMPD_CACHE_DISK_LYRICS_H
#define MYMPD_CACHE_DISK_LYRICS_H

#include "dist/sds/sds.h"

#include <stdbool.h>

sds cache_disk_lyrics_get_name(const char *cachedir, const char *uri);
sds cache_disk_lyrics_write_file(const char *cachedir, const char *uri, const char *str);

#endif
