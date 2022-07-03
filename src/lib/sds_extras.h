/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2022 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#ifndef MYMPD_SDS_EXTRAS_H
#define MYMPD_SDS_EXTRAS_H

#include "../../dist/sds/sds.h"

#include <stdbool.h>
#include <stdio.h>

#define FREE_SDS(SDS_PTR) do { \
    sdsfree(SDS_PTR); \
    SDS_PTR = NULL; \
} while (0)

int sds_toimax(sds s);
void sds_utf8_tolower(sds s);
sds sds_catjson_plain(sds s, const char *p, size_t len);
sds sds_catjson(sds s, const char *p, size_t len);
sds sds_catjsonchar(sds s, const char p);
sds sds_catchar(sds s, const char p);
bool sds_json_unescape(const char *src, size_t slen, sds *dst);
sds sds_urldecode(sds s, const char *p, size_t len, bool is_form_url_encoded);
sds sds_urlencode(sds s, const char *p, size_t len);
sds sds_replacelen(sds s, const char *value, size_t len);
sds sds_replace(sds s, const char *value);
int sds_getline(sds *s, FILE *fp, size_t max);
int sds_getline_n(sds *s, FILE *fp, size_t max);
int sds_getfile(sds *s, FILE *fp, size_t max);
void sds_basename_uri(sds uri);
void sds_strip_file_extension(sds s);
void sds_strip_slash(sds s);
sds sds_catbool(sds s, bool v);
void sds_sanitize_filename(sds s);
sds sds_hash(const char *p);

#endif
