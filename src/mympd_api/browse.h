/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2023 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#ifndef MYMPD_API_BROWSE_H
#define MYMPD_API_BROWSE_H

#include "src/lib/mympd_state.h"

sds mympd_api_browse_album_detail(struct t_mympd_state *mympd_state, struct t_partition_state *partition_state,
        sds buffer, unsigned request_id, sds albumid, const struct t_tags *tagcols);
sds mympd_api_browse_album_list(struct t_partition_state *partition_state, struct t_cache *album_cache,
        sds buffer, unsigned request_id, sds expression, sds sort, bool sortdesc, unsigned offset, unsigned limit,
        const struct t_tags *tagcols);
sds mympd_api_browse_tag_list(struct t_partition_state *partition_state, sds buffer,
        unsigned request_id, sds searchstr, sds tag, unsigned offset, unsigned limit, bool sortdesc);
#endif
