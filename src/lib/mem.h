/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2026 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#ifndef MYMPD_MEM_H
#define MYMPD_MEM_H

#include "src/lib/log.h"

#include <stdlib.h>

char *my_strdup(const char *str, size_t len);

/**
 * Calls malloc and aborts if it fails
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
 * Calls malloc and aborts if it fails
 * @param count number of elements
 * @param size element size in bytes
 * @return malloced pointer
 */
__attribute__((malloc))
static inline void *malloc_assert_count(size_t count, size_t size) {
    if (count != 0 && size > SIZE_MAX / count) {
        MYMPD_LOG_EMERG(NULL, "Overflow in allocating call for %lu x %lu", (unsigned long) count, (unsigned long) size);
        abort();
    }
    void *p = malloc(count * size);
    if (p == NULL) {
        MYMPD_LOG_EMERG(NULL, "Failure allocating %lu bytes of memory", (unsigned long) (count * size));
        abort();
    }
    return p;
}

/**
 * Calls calloc and aborts if it fails
 * @param count number of elements
 * @param size element size in bytes
 * @return malloced pointer
 */
__attribute__((malloc))
static inline void *calloc_assert(size_t count, size_t size) {
    if (count != 0 && size > SIZE_MAX / count) {
        MYMPD_LOG_EMERG(NULL, "Overflow in allocating call for %lu x %lu", (unsigned long) count, (unsigned long) size);
        abort();
    }
    void *p = calloc(count, size);
    if (p == NULL) {
        MYMPD_LOG_EMERG(NULL, "Failure allocating %lu bytes of memory", (unsigned long) (count * size));
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
 * Calls realloc and aborts if it fails
 * @param ptr pointer to resize
 * @param count number of elements
 * @param size element size in bytes
 * @return reallocated pointer
 */
__attribute__((malloc))
static inline void *realloc_assert_count(void *ptr, size_t count, size_t size) {
    if (count != 0 && size > SIZE_MAX / count) {
        MYMPD_LOG_EMERG(NULL, "Overflow in re-allocating call for %lu x %lu", (unsigned long) count, (unsigned long) size);
        abort();
    }
    void *p = realloc(ptr, count * size);
    if (p == NULL) {
        MYMPD_LOG_EMERG(NULL, "Failure allocating %lu bytes of memory", (unsigned long) (count * size));
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
