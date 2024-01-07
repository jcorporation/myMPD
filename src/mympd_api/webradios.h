/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2024 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#ifndef MYMPD_API_WEBRADIOS_H
#define MYMPD_API_WEBRADIOS_H

#include "dist/sds/sds.h"
#include "src/lib/list.h"

#include <stdbool.h>

sds get_webradio_from_uri(sds workdir, const char *uri);
bool mympd_api_webradio_save(sds workdir, sds name, sds uri, sds uri_old,
        sds genre, sds picture, sds homepage, sds country, sds language,
        sds codec, int bitrate, sds description, sds state);
bool mympd_api_webradio_delete(sds workdir, struct t_list *filenames);
sds mympd_api_webradio_get(sds workdir, sds buffer, unsigned request_id, sds filename);
sds mympd_api_webradio_list(sds workdir, sds buffer, unsigned request_id, sds searchstr,
        unsigned offset, unsigned limit);

#endif
