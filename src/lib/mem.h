/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2025 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#ifndef MYMPD_MEM_H
#define MYMPD_MEM_H

#include "src/lib/log.h"

#include <stdlib.h>

/**
 * Calls malloc and abort if it fails
 * @param size bytes to malloc
 * @return malloced pointer
 */
__attribute__((malloc))
static inline void *malloc_assert(size_t size) {
    void *p = malloc(size);
    if (p == NULL) {
        MYMPD_LOG_EMERG(NULL, "Failure allocating %lu bytes of memory", (unsigned long) size);
        abort();
    }
    return p;
}

/**
 * Calls realloc and aborts if it fails
 * @param ptr pointer to resize
 * @param size bytes to realloc
 * @return reallocated pointer
 */
__attribute__((malloc))
static inline void *realloc_assert(void *ptr, size_t size) {
    void *p = realloc(ptr, size);
    if (p == NULL) {
        MYMPD_LOG_EMERG(NULL, "Failure allocating %lu bytes of memory", (unsigned long) size);
        abort();
    }
    return p;
}

/**
 * Macro to free the pointer and set it to NULL
 * @param PTR pointer to free
 */
#define FREE_PTR(PTR) do { \
    if (PTR != NULL) \
        free(PTR); \
    PTR = NULL; \
} while (0)

#endif
