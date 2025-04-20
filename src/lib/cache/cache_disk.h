/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2025 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

/*! \file
 * \brief General disk cache handling
 */

#ifndef MYMPD_CACHE_DISK_H
#define MYMPD_CACHE_DISK_H

#include "src/lib/config_def.h"

#include <stdbool.h>

/**
 * Disk cache config
 */
enum cache_disk_conf {
    CACHE_DISK_NO_PRUNE = -1,   //!< Do not prune the cache
    CACHE_DISK_DISABLED = 0     //!< Cache is disbled
};

void cache_disk_clear(struct t_config *config);
void cache_disk_crop(struct t_config *config);

#endif
