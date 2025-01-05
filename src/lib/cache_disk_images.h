/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2025 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

/*! \file
 * \brief Image cache handling
 */

#ifndef MYMPD_CACHE_DISK_IMAGES_H
#define MYMPD_CACHE_DISK_IMAGES_H

#include "dist/sds/sds.h"

#include <stdbool.h>

sds cache_disk_images_get_basename(const char *cachedir, const char *type, const char *uri, int offset);
sds cache_disk_images_write_file(sds cachedir,  const char *type, const char *uri, const char *mime_type, sds binary, int offset);

#endif
