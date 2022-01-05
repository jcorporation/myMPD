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

void sds_utf8_tolower(sds s);
sds sds_catjson(sds s, const char *p, size_t len);
sds sds_catjsonchar(sds s, const char p);
bool sds_json_unescape(const char *src, int slen, sds *dst);
sds sds_urldecode(sds s, const char *p, size_t len, int is_form_url_encoded);
sds sds_urlencode(sds s, const char *p, size_t len);
sds sds_replacelen(sds s, const char *value, size_t len);
sds sds_replace(sds s, const char *value);
int sds_getline(sds *s, FILE *fp, size_t max);
int sds_getline_n(sds *s, FILE *fp, size_t max);
int sds_getfile(sds *s, FILE *fp, size_t max);
sds sds_get_extension_from_filename(const char *filename);
void sds_basename_uri(sds uri);
void sds_strip_file_extension(sds s);
void sds_strip_slash(sds s);
sds sds_catbool(sds s, bool v);
void sds_sanitize_filename(sds s);

#endif
