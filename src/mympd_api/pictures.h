/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2022 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#ifndef MYMPD_API_PICTURES_H
#define MYMPD_API_PICTURES_H

#include "dist/sds/sds.h"

sds mympd_api_settings_picture_list(sds workdir, sds buffer, long request_id, sds subdir);

#endif
