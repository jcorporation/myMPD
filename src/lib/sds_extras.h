/*
 SPDX-License-Identifier: GPL-2.0-or-later
 myMPD (c) 2018-2021 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#ifndef MYMPD_SDS_EXTRAS_H
#define MYMPD_SDS_EXTRAS_H

#include "../../dist/src/sds/sds.h"

#include <stdbool.h>
#include <stdio.h>

#define FREE_SDS(SDS) do { \
    if (SDS != NULL) \
        sdsfree(SDS); \
    SDS = NULL; \
} while (0)

sds sdscatjson(sds s, const char *p, size_t len);
sds sdscatjsonchar(sds s, const char p);
bool sds_json_unescape(const char *src, int slen, sds *dst);
sds sdsurldecode(sds s, const char *p, size_t len, int is_form_url_encoded);
sds sdscrop(sds s);
sds sdsreplacelen(sds s, const char *value, size_t len);
sds sdsreplace(sds s, const char *value);
int sdsgetline(sds *s, FILE *fp, size_t max);
int sdsgetline_n(sds *s, FILE *fp, size_t max);
int sdsgetfile(sds *s, FILE *fp, size_t max);
#endif
