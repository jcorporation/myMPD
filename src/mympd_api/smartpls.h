/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2023 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#ifndef MYMPD_API_SMARTPLS_H
#define MYMPD_API_SMARTPLS_H

#include "dist/sds/sds.h"

sds mympd_api_smartpls_get(sds workdir, sds buffer, unsigned request_id, const char *playlist);

#endif
