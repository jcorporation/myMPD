/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2024 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

/*! \file
 * \brief myMPD status API
 */

#ifndef MYMPD_API_STATUS_H
#define MYMPD_API_STATUS_H

#include "src/lib/jsonrpc.h"
#include "src/lib/mympd_state.h"

unsigned mympd_api_get_elapsed_seconds(struct mpd_status *status);
sds mympd_api_status_print(struct t_partition_state *partition_state, struct t_cache *album_cache, sds buffer, struct mpd_status *status);
sds mympd_api_status_updatedb_state(struct t_partition_state *partition_state, sds buffer);
unsigned mympd_api_status_updatedb_id(struct t_partition_state *partition_state);
sds mympd_api_status_volume_get(struct t_partition_state *partition_state, sds buffer, unsigned request_id, enum jsonrpc_response_types response_type);
sds mympd_api_status_get(struct t_partition_state *partition_state, struct t_cache *album_cache,
        sds buffer, unsigned request_id, enum jsonrpc_response_types response_type);
bool mympd_api_status_clear_error(struct t_partition_state *partition_state, sds *buffer, enum mympd_cmd_ids cmd_id, unsigned request_id);
sds mympd_api_status_current_song(struct t_mympd_state *mympd_state, struct t_partition_state *partition_state,
        sds buffer, unsigned request_id);

#endif
