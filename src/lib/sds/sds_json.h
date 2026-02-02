/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2026 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

/*! \file
 * \brief JSON functions for sds strings
 */

#ifndef MYMPD_SDS_JSON_H
#define MYMPD_SDS_JSON_H

#include "dist/sds/sds.h"

#include <stdbool.h>
#include <stdio.h>

sds sds_catjson_plain(sds s, const char *p, size_t len);
sds sds_catjson(sds s, const char *p, size_t len);
sds sds_catjsonchar(sds s, const char c);
bool sds_json_unescape(const char *src, size_t slen, sds *dst);

#endif
