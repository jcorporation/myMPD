/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2023 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#ifndef MYMPD_CACHE_H
#define MYMPD_CACHE_H

#include "dist/rax/rax.h"

#include <pthread.h>
#include <stdbool.h>

/**
 * Holds cache information
 */
struct t_cache {
    bool building;             //!< true if the mpd_worker thread is creating the cache
    rax *cache;                //!< pointer to the cache
    pthread_rwlock_t *rwlock;  //!< pthreads read-write lock object
};

void cache_init(struct t_cache *cache);
void cache_free(struct t_cache *cache);

bool cache_get_read_lock(struct t_cache *cache);
bool cache_get_write_lock(struct t_cache *cache);
bool cache_free_lock(struct t_cache *cache);

#endif
