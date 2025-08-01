/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2025 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

/*! \file
 * \brief myMPD browse database API
 */

#ifndef MYMPD_API_TAGS_H
#define MYMPD_API_TAGS_H

#include "src/lib/mympd_state.h"

sds mympd_api_tag_list(struct t_partition_state *partition_state, sds buffer,
        unsigned request_id, sds searchstr, sds tag, unsigned offset, unsigned limit, bool sortdesc);

#endif
