/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2025 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

/*! \file
 * \brief General rax cache handling
 */

#ifndef MYMPD_CACHE_RAX_H
#define MYMPD_CACHE_RAX_H

#include "dist/rax/rax.h"

#include <pthread.h>
#include <stdbool.h>

/**
 * Holds cache information
 */
struct t_cache {
    bool building;             //!< true if the mympd_worker thread is creating the cache
    rax *cache;                //!< pointer to the cache
    pthread_rwlock_t rwlock;   //!< pthreads read-write lock object
};

bool cache_init(struct t_cache *cache);
bool cache_free(struct t_cache *cache);

bool cache_get_read_lock(struct t_cache *cache);
bool cache_get_write_lock(struct t_cache *cache);
bool cache_release_lock(struct t_cache *cache);

#endif
