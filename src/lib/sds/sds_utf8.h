/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2026 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

/*! \file
 * \brief UTF8 functions for sds strings
 */

#ifndef MYMPD_SDS_UTF8_H
#define MYMPD_SDS_UTF8_H

#include "dist/sds/sds.h"

sds sds_utf8_casefold(sds s);
sds sds_utf8_normalize(sds s);

#endif
