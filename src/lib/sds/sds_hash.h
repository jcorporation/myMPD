/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2026 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

/*! \file
 * \brief Hash functions for sds strings
 */

#ifndef MYMPD_SDS_HASH_H
#define MYMPD_SDS_HASH_H

#include "dist/sds/sds.h"

sds sds_hash_md5(const char *p);
sds sds_hash_sha1(const char *p);
sds sds_hash_sha1_sds(sds s);
sds sds_hash_sha256(const char *p);
sds sds_hash_sha256_sds(sds s);

#endif
