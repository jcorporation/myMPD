/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2022 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#ifndef MYMPD_API_WEBRADIOS_H
#define MYMPD_API_WEBRADIOS_H

#include "../../dist/sds/sds.h"
#include "../lib/mympd_configuration.h"

#include <stdbool.h>

sds get_webradio_from_uri(struct t_config *config, const char *uri);
bool mympd_api_webradio_save(struct t_config *config, sds name, sds uri, sds uri_old,
        sds genre, sds picture, sds homepage, sds country, sds language, sds description);
bool mympd_api_webradio_delete(struct t_config *config, const char *filename);
sds mympd_api_webradio_get(struct t_config *config, sds buffer, sds method, long request_id,
        const char *filename);
sds mympd_api_webradio_list(struct t_config *config, sds buffer, sds method, long request_id,
        sds searchstr, long offset, long limit);

#endif
