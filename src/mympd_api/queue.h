/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2024 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

/*! \file
 * \brief myMPD queue API
 */

#ifndef MYMPD_API_QUEUE_H
#define MYMPD_API_QUEUE_H

#include "src/lib/api.h"
#include "src/lib/mympd_state.h"

bool mympd_api_queue_save(struct t_partition_state *partition_state, sds name, sds mode, sds *error);
sds mympd_api_queue_list(struct t_mympd_state *mympd_state, struct t_partition_state *partition_state,
        sds buffer, unsigned request_id, unsigned offset, unsigned limit, const struct t_fields *tagcols);
sds mympd_api_queue_search(struct t_mympd_state *mympd_state, struct t_partition_state *partition_state,
        sds buffer, unsigned request_id, sds expression, sds sort, bool sortdesc, unsigned offset, unsigned limit,
        const struct t_fields *tagcols);
sds mympd_api_queue_crop(struct t_partition_state *partition_state, sds buffer, enum mympd_cmd_ids cmd_id,
        unsigned request_id, bool or_clear);
bool mympd_api_queue_prio_set(struct t_partition_state *partition_state, struct t_list *song_ids, unsigned priority, sds *error);
bool mympd_api_queue_prio_set_highest(struct t_partition_state *partition_state, struct t_list *song_ids, sds *error);
bool mympd_api_queue_rm_song_ids(struct t_partition_state *partition_state, struct t_list *song_ids, sds *error);
bool mympd_api_queue_append_uri_tags(struct t_partition_state *partition_state, sds uri, struct t_list *tags, sds *error);
bool mympd_api_queue_insert_uri_tags(struct t_partition_state *partition_state, sds uri,
        struct t_list *tags, unsigned to, unsigned whence, sds *error);
bool mympd_api_queue_insert_uri_resume(struct t_partition_state *partition_state, struct t_stickerdb_state *stickerdb,
        sds uri, unsigned to, unsigned whence, sds *error);
bool mympd_api_queue_append_uri_resume(struct t_partition_state *partition_state, struct t_stickerdb_state *stickerdb,
        sds uri, sds *error);
bool mympd_api_queue_replace_uri_resume(struct t_partition_state *partition_state, struct t_stickerdb_state *stickerdb,
        sds uri, sds *error);
bool mympd_api_queue_replace_uri_tags(struct t_partition_state *partition_state, sds uri, struct t_list *tags, sds *error);
bool mympd_api_queue_append(struct t_partition_state *partition_state, struct t_list *uris, sds *error);
bool mympd_api_queue_insert(struct t_partition_state *partition_state, struct t_list *uris, unsigned to, unsigned whence, sds *error);
bool mympd_api_queue_replace(struct t_partition_state *partition_state, struct t_list *uris, sds *error);
bool mympd_api_queue_insert_search(struct t_partition_state *partition_state, sds expression,
        unsigned to, unsigned whence, const char *sort, bool sort_desc, sds *error);
bool mympd_api_queue_append_search(struct t_partition_state *partition_state, sds expression,
        const char *sort, bool sort_desc, sds *error);
bool mympd_api_queue_replace_search(struct t_partition_state *partition_state, sds expression,
        const char *sort, bool sort_desc, sds *error);
bool mympd_api_queue_insert_plist_range(struct t_partition_state *partition_state, sds plist,
        unsigned to, unsigned whence, unsigned start, int end, sds *error);
bool mympd_api_queue_append_plist_range(struct t_partition_state *partition_state, sds plist,
        unsigned start, int end, sds *error);
bool mympd_api_queue_replace_plist_range(struct t_partition_state *partition_state, sds plist,
        unsigned start, int end, sds *error);
bool mympd_api_queue_append_plists(struct t_partition_state *partition_state, struct t_list *plists, sds *error);
bool mympd_api_queue_insert_plists(struct t_partition_state *partition_state, struct t_list *plists, unsigned to, unsigned whence, sds *error);
bool mympd_api_queue_replace_plists(struct t_partition_state *partition_state, struct t_list *plists, sds *error);
bool mympd_api_queue_move_relative(struct t_partition_state *partition_state, struct t_list *song_ids, unsigned to, unsigned whence, sds *error);
bool mympd_api_queue_append_albums(struct t_partition_state *partition_state, struct t_cache *album_cache,
        struct t_list *albumids, sds *error);
bool mympd_api_queue_insert_albums(struct t_partition_state *partition_state, struct t_cache *album_cache,
        struct t_list *albumids, unsigned to, unsigned whence, sds *error);
bool mympd_api_queue_replace_albums(struct t_partition_state *partition_state, struct t_cache *album_cache,
        struct t_list *albumids, sds *error);
bool mympd_api_queue_append_album_tag(struct t_partition_state *partition_state, struct t_cache *album_cache,
        sds albumid, enum mpd_tag_type tag, sds tag_value, sds *error);
bool mympd_api_queue_insert_album_tag(struct t_partition_state *partition_state, struct t_cache *album_cache,
        sds albumid, enum mpd_tag_type tag, sds tag_value, unsigned to, unsigned whence, sds *error);
bool mympd_api_queue_replace_album_tag(struct t_partition_state *partition_state, struct t_cache *album_cache,
        sds albumid, enum mpd_tag_type tag, sds tag_value, sds *error);
bool mympd_api_queue_insert_album_range(struct t_partition_state *partition_state, struct t_cache *album_cache,
        sds albumid, unsigned start, int end, unsigned to, unsigned whence, sds *error);
bool mympd_api_queue_append_album_range(struct t_partition_state *partition_state, struct t_cache *album_cache,
        sds albumid, unsigned start, int end, sds *error);
bool mympd_api_queue_replace_album_range(struct t_partition_state *partition_state, struct t_cache *album_cache,
        sds albumid, unsigned start, int end, sds *error);

#endif
