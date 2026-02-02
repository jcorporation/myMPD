/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2026 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

/*! \file
 * \brief URL functions for sds strings
 */

#ifndef MYMPD_SDS_URL_H
#define MYMPD_SDS_URL_H

#include "dist/sds/sds.h"

#include <stdbool.h>

sds sds_urldecode(sds s, const char *p, size_t len, bool is_form_url_encoded);
sds sds_urlencode(sds s, const char *p, size_t len);

#endif
