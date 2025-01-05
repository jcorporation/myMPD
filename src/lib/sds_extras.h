/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2025 Juergen Mang <mail@jcgames.de>
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

sds sds_basename(sds s);
sds sds_dirname(sds s);
sds *sds_split_comma_trim(sds s, int *count);
void sds_utf8_tolower(sds s);
sds sds_catjson_plain(sds s, const char *p, size_t len);
sds sds_catjson(sds s, const char *p, size_t len);
sds sds_catjsonchar(sds s, const char c);
sds sds_catchar(sds s, const char c);
bool sds_json_unescape(const char *src, size_t slen, sds *dst);
sds sds_urldecode(sds s, const char *p, size_t len, bool is_form_url_encoded);
sds sds_urlencode(sds s, const char *p, size_t len);
sds sds_replacelen(sds s, const char *p, size_t len);
sds sds_replace(sds s, const char *p);
sds sds_catbool(sds s, bool v);
sds sds_hash_md5(const char *p);
sds sds_hash_sha1(const char *p);
sds sds_hash_sha1_sds(sds s);
sds sds_hash_sha256(const char *p);
sds sds_hash_sha256_sds(sds s);
sds sds_pad_int(int64_t value, sds buffer);

#endif
