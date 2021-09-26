/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2021 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include "mympd_config_defs.h"
#include "mem.h"

#include <assert.h>

void *malloc_assert(size_t size) {
    void *p = malloc(size);
    assert(p);
    return p;
}

void *realloc_assert(void *ptr, size_t size) {
    void *p = realloc(ptr, size);
    assert(p);
    return p;
}
