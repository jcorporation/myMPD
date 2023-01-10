/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2023 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#ifndef MYMPD_MIMETYPE_H
#define MYMPD_MIMETYPE_H

#include "dist/sds/sds.h"

#include <stdbool.h>

const char *get_mime_type_by_ext(const char *filename);
const char *get_ext_by_mime_type(const char *mime_type);
const char *get_mime_type_by_magic_stream(sds stream);
bool is_image(const char *filename);

#endif
