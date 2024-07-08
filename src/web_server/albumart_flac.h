/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2024 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

/*! \file
 * \brief Embedded image functions for FLAC
 */

#ifndef MYMPD_WEB_SERVER_ALBUMART_FLAC_H
#define MYMPD_WEB_SERVER_ALBUMART_FLAC_H

#include "dist/sds/sds.h"
#include <stdbool.h>

bool handle_coverextract_flac(sds cachedir, const char *uri, const char *media_file, sds *binary, bool is_ogg, bool covercache, int offset);

#endif
