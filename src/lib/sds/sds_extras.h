/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2026 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

/*! \file
 * \brief Extra functions for sds strings
 */

#ifndef MYMPD_SDS_EXTRAS_H
#define MYMPD_SDS_EXTRAS_H

#include "dist/sds/sds.h"

#include <stdbool.h>
#include <stdio.h>

/**
 * Frees an sds string and sets it to NULL
 */
#define FREE_SDS(SDS_PTR) do { \
    sdsfree(SDS_PTR); \
    SDS_PTR = NULL; \
} while (0)

sds *sds_split_comma_trim(const char *p, int *count);
sds sds_catchar(sds s, const char c);
sds sds_replacelen(sds s, const char *p, size_t len);
sds sds_replace(sds s, const char *p);
sds sds_catbool(sds s, bool v);
sds sds_pad_int(int64_t value, sds buffer);
void sds_free_void(void *p);

#endif
