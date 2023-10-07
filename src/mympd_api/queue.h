/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2023 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#ifndef MYMPD_API_QUEUE_H
#define MYMPD_API_QUEUE_H

#include "src/lib/api.h"
#include "src/lib/mympd_state.h"

bool mympd_api_queue_save(struct t_partition_state *partition_state, sds name, sds mode, sds *error);
sds mympd_api_queue_list(struct t_partition_state *partition_state, sds buffer, long request_id,
        unsigned offset, unsigned limit, const struct t_tags *tagcols);
sds mympd_api_queue_crop(struct t_partition_state *partition_state, sds buffer, enum mympd_cmd_ids cmd_id,
        long request_id, bool or_clear);
sds mympd_api_queue_search(struct t_partition_state *partition_state, sds buffer, long request_id,
        sds expression, sds sort, bool sortdesc, unsigned offset, unsigned limit,
        const struct t_tags *tagcols);
bool mympd_api_queue_prio_set(struct t_partition_state *partition_state, struct t_list *song_ids, unsigned priority, sds *error);
bool mympd_api_queue_prio_set_highest(struct t_partition_state *partition_state, struct t_list *song_ids, sds *error);
bool mympd_api_queue_rm_song_ids(struct t_partition_state *partition_state, struct t_list *song_ids, sds *error);
bool mympd_api_queue_append(struct t_partition_state *partition_state, struct t_list *uris, sds *error);
bool mympd_api_queue_insert(struct t_partition_state *partition_state, struct t_list *uris, unsigned to, unsigned whence, sds *error);
bool mympd_api_queue_replace(struct t_partition_state *partition_state, struct t_list *uris, sds *error);
bool mympd_api_queue_insert_search(struct t_partition_state *partition_state, sds expression,
        unsigned to, unsigned whence, const char *sort, bool sort_desc, sds *error);
bool mympd_api_queue_append_search(struct t_partition_state *partition_state, sds expression,
        const char *sort, bool sort_desc, sds *error);
bool mympd_api_queue_replace_search(struct t_partition_state *partition_state, sds expression,
        const char *sort, bool sort_desc, sds *error);
bool mympd_api_queue_append_plist(struct t_partition_state *partition_state, struct t_list *plists, sds *error);
bool mympd_api_queue_insert_plist(struct t_partition_state *partition_state, struct t_list *plists, unsigned to, unsigned whence, sds *error);
bool mympd_api_queue_replace_plist(struct t_partition_state *partition_state, struct t_list *plists, sds *error);
bool mympd_api_queue_move_relative(struct t_partition_state *partition_state, struct t_list *song_ids, unsigned to, unsigned whence, sds *error);
bool mympd_api_queue_append_albums(struct t_partition_state *partition_state, struct t_list *albumids, sds *error);
bool mympd_api_queue_insert_albums(struct t_partition_state *partition_state, struct t_list *albumids, unsigned to, unsigned whence, sds *error);
bool mympd_api_queue_replace_albums(struct t_partition_state *partition_state, struct t_list *albumids, sds *error);
bool mympd_api_queue_append_album_disc(struct t_partition_state *partition_state, sds albumid, sds disc, sds *error);
bool mympd_api_queue_insert_album_disc(struct t_partition_state *partition_state, sds albumid, sds disc, unsigned to, unsigned whence, sds *error);
bool mympd_api_queue_replace_album_disc(struct t_partition_state *partition_state, sds albumid, sds disc, sds *error);
#endif
