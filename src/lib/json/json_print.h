/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2025 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

/*! \file
 * \brief Json print implementation for sds
 */

#ifndef MYMPD_JSON_PRINT_H
#define MYMPD_JSON_PRINT_H

#include "dist/sds/sds.h"

#include <stdbool.h>

sds json_comma(sds buffer);
sds tojson_raw(sds buffer, const char *key, const char *value, bool comma);
sds tojson_sds(sds buffer, const char *key, sds value, bool comma);
sds tojson_char(sds buffer, const char *key, const char *value, bool comma);
sds tojson_char_len(sds buffer, const char *key, const char *value, size_t len, bool comma);
sds tojson_bool(sds buffer, const char *key, bool value, bool comma);
sds tojson_int(sds buffer, const char *key, int value, bool comma);
sds tojson_uint(sds buffer, const char *key, unsigned value, bool comma);
sds tojson_time(sds buffer, const char *key, time_t value, bool comma);
sds tojson_float(sds buffer, const char *key, float value, bool comma);
sds tojson_int64(sds buffer, const char *key, int64_t value, bool comma);
sds tojson_uint64(sds buffer, const char *key, uint64_t value, bool comma);

#endif
