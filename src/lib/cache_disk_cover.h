/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2024 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#ifndef MYMPD_CACHE_DISK_COVER_H
#define MYMPD_CACHE_DISK_COVER_H

#include "dist/sds/sds.h"

#include <stdbool.h>

sds cache_disk_cover_get_basename(const char *cachedir, const char *uri, int offset);
bool cache_disk_cover_write_file(sds cachedir, const char *uri, const char *mime_type, sds binary, int offset);

#endif
