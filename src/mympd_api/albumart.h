/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2024 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

/*! \file
 * \brief myMPD Albumart API
 */

#ifndef MYMPD_API_COVER_H
#define MYMPD_API_COVER_H

#include "src/lib/mympd_state.h"

sds mympd_api_albumart_getcover_by_album_id(struct t_partition_state *partition_state, struct t_cache *album_cache,
        sds buffer, unsigned request_id, sds albumid, unsigned size);
sds mympd_api_albumart_getcover_by_uri(struct t_mympd_state *mympd_state, struct t_partition_state *partition_state,
    sds buffer, unsigned request_id, unsigned long conn_id, sds uri, sds *binary);

#endif
