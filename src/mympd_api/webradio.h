/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2024 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

/*! \file
 * \brief myMPD API webradio functions
 */

#ifndef MYMPD_API_WEBRADIO_H
#define MYMPD_API_WEBRADIO_H

#include "dist/sds/sds.h"
#include "src/lib/api.h"
#include "src/lib/mympd_state.h"

sds mympd_api_webradio_search(struct t_webradios *webradios, sds buffer, unsigned request_id,
    enum mympd_cmd_ids cmd_id, unsigned offset, unsigned limit, sds expression, sds sort, bool sortdesc);
sds mympd_api_webradio_radio_get_by_name(struct t_webradios *webradios, sds buffer, unsigned request_id,
    enum mympd_cmd_ids cmd_id, sds name);
sds mympd_api_webradio_radio_get_by_uri(struct t_webradios *webradios, sds buffer, unsigned request_id,
    enum mympd_cmd_ids cmd_id, sds uri);
sds mympd_api_webradio_from_uri_tojson(struct t_mympd_state *mympd_state, const char *uri);
sds mympd_api_webradio_print(struct t_webradio_data *webradio, sds buffer);

#endif
