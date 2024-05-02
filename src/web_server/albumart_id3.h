/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2024 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#ifndef MYMPD_WEB_SERVER_ALBUMART_ID3_H
#define MYMPD_WEB_SERVER_ALBUMART_ID3_H

#include "dist/sds/sds.h"
#include <stdbool.h>

bool handle_coverextract_id3(sds cachedir, const char *uri, const char *media_file, sds *binary, bool covercache, int offset);

#endif
