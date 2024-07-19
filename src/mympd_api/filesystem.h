/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2024 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

/*! \file
 * \brief myMPD filesystem API
 */

#ifndef MYMPD_API_FILESYSTEM_H
#define MYMPD_API_FILESYSTEM_H

#include "src/lib/mympd_state.h"

sds mympd_api_browse_filesystem(struct t_mympd_state *mympd_state, struct t_partition_state *partition_state,
        sds buffer, unsigned request_id, sds path, unsigned offset, unsigned limit, sds searchstr, const struct t_fields *tagcols);
#endif
