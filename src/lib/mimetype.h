/*
 SPDX-License-Identifier: GPL-2.0-or-later
 myMPD (c) 2018-2021 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#ifndef MYMPD_MIMETYPE_H
#define MYMPD_MIMETYPE_H

#include "../../dist/src/sds/sds.h"

sds find_image_file(sds basefilename);
sds get_mime_type_by_ext(const char *filename);
sds get_ext_by_mime_type(const char *mime_type);
sds get_mime_type_by_magic(const char *filename);
sds get_mime_type_by_magic_stream(sds stream);
sds get_extension_from_filename(const char *filename);

#endif
