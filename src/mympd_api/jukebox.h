/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2024 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#ifndef MYMPD_API_JUKEBOX_H
#define MYMPD_API_JUKEBOX_H

#include "src/lib/api.h"
#include "src/lib/mympd_state.h"

void mympd_api_jukebox_clear(struct t_list *list, sds partition_name);
bool mympd_api_jukebox_rm_entries(struct t_list *list, struct t_list *positions, sds partition_name, sds *error);
sds mympd_api_jukebox_list(struct t_partition_state *partition_state, struct t_stickerdb_state *stickerdb,
        sds buffer, enum mympd_cmd_ids cmd_id, unsigned request_id, unsigned offset, unsigned limit, sds expression, const struct t_tags *tagcols);
#endif
