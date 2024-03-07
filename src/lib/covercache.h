/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2024 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#ifndef MYMPD_COVERCACHE_H
#define MYMPD_COVERCACHE_H

#include "dist/sds/sds.h"

#include <stdbool.h>

enum covercache_conf {
    COVERCACHE_NO_PRUNE = -1,
    COVERCACHE_DISABLED = 0
};

bool covercache_write_file(sds cachedir, const char *uri, const char *mime_type, sds binary, int offset);
int covercache_clear(sds cachedir, int keepdays);
#endif
