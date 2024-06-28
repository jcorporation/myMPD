/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2024 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#ifndef MYMPD_CACHE_DISK_H
#define MYMPD_CACHE_DISK_H

#include "src/lib/config_def.h"

#include <stdbool.h>

enum cache_disk_conf {
    CACHE_DISK_NO_PRUNE = -1,
    CACHE_DISK_DISABLED = 0
};

void cache_disk_clear(struct t_config *config);
void cache_disk_crop(struct t_config *config);

#endif
