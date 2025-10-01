/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2025 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

/*! \file
 * \brief Add random functions
 */

#include "compile_time.h"
#include "src/mympd_worker/random_select.h"

#include "dist/sds/sds.h"
#include "src/lib/cache/cache_rax.h"
#include "src/lib/json/json_print.h"
#include "src/lib/json/json_rpc.h"
#include "src/lib/log.h"
#include "src/lib/sds_extras.h"
#include "src/mympd_client/errorhandler.h"
#include "src/mympd_client/queue.h"
#include "src/mympd_client/random_select.h"
#include "src/mympd_client/shortcuts.h"
#include "src/mympd_client/tags.h"

#include <stdbool.h>
#include <string.h>

/**
 * Adds randoms songs or albums to the queue
 * @param mympd_worker_state pointer to mympd_worker_state
 * @param add number of songs/albums to add
 * @param mode 1 = add songs, 2 = add albums
 * @param plist playlist to select songs from
 * @param play Play the inserted songs?
 * @param partition partition to add the selection
 * @return true on success, else false
 */
bool mympd_worker_add_random_to_queue(struct t_mympd_worker_state *mympd_worker_state,
        unsigned add, unsigned mode, sds plist, bool play, sds partition)
{
    struct t_random_add_constraints constraints = {
        .filter_include = NULL,
        .filter_exclude = NULL,
        .uniq_tag = MPD_TAG_UNKNOWN,
        .last_played = 0,
        .ignore_hated = false,
        .min_song_duration = 0,
        .max_song_duration = UINT_MAX
    };

    struct t_list add_list;
    list_init(&add_list);

    unsigned new_length = 0;
    sds error = sdsempty();
    if (mode == JUKEBOX_ADD_ALBUM) {
        if (cache_get_read_lock(mympd_worker_state->album_cache) == true) {
            new_length = random_select_albums(mympd_worker_state->partition_state, mympd_worker_state->stickerdb,
                mympd_worker_state->album_cache, add, NULL, &add_list, &constraints);
            if (new_length > 0) {
                mympd_client_add_albums_to_queue(mympd_worker_state->partition_state, mympd_worker_state->album_cache, &add_list,
                    UINT_MAX, MPD_POSITION_ABSOLUTE, &error);
            }
            cache_release_lock(mympd_worker_state->album_cache);
        }
    }
    else if  (mode == JUKEBOX_ADD_SONG){
        new_length = random_select_songs(mympd_worker_state->partition_state, mympd_worker_state->stickerdb,
            add, plist, NULL, &add_list, &constraints);
        if (new_length > 0) {
            mympd_client_add_uris_to_queue(mympd_worker_state->partition_state, &add_list, UINT_MAX, MPD_POSITION_ABSOLUTE, &error);
        }
    }
    else {
        MYMPD_LOG_WARN(partition, "Invalid jukebox mode");
        FREE_SDS(error);
        list_clear(&add_list);
        return false;
    }
    list_clear(&add_list);

    if (new_length < add) {
        MYMPD_LOG_WARN(partition, "Could not select %u entries", add);
        FREE_SDS(error);
        return false;
    }

    if (mympd_client_queue_check_start_play(mympd_worker_state->partition_state, play, &error) == false) {
        FREE_SDS(error);
        return false;
    }
    FREE_SDS(error);
    return true;
}

/**
 * Lists randoms songs or albums
 * @param mympd_worker_state pointer to mympd_worker_state
 * @param buffer Already allocated sds string to append the response
 * @param request_id Jsonrpc id
 * @param quantity number of songs/albums to add
 * @param mode 1 = add songs, 2 = add albums
 * @param plist playlist to select songs from
 * @return Pointer to buffer
 */
sds mympd_worker_list_random(struct t_mympd_worker_state *mympd_worker_state, sds buffer, unsigned request_id,
        unsigned quantity, unsigned mode, sds plist)
{
    struct t_random_add_constraints constraints = {
        .filter_include = NULL,
        .filter_exclude = NULL,
        .uniq_tag = MPD_TAG_UNKNOWN,
        .last_played = 0,
        .ignore_hated = false,
        .min_song_duration = 0,
        .max_song_duration = UINT_MAX
    };

    struct t_list add_list;
    list_init(&add_list);

    sds error = sdsempty();
    if (mode == JUKEBOX_ADD_ALBUM) {
        if (cache_get_read_lock(mympd_worker_state->album_cache) == true) {
            random_select_albums(mympd_worker_state->partition_state, mympd_worker_state->stickerdb,
                mympd_worker_state->album_cache, quantity, NULL, &add_list, &constraints);
            buffer = jsonrpc_respond_start(buffer, MYMPD_API_DATABASE_LIST_RANDOM, request_id);
            buffer = sdscat(buffer, "\"data\":[");
            struct t_list_node *current = add_list.head;
            while (current != NULL) {
                buffer = sdscat(buffer, "{\"Type\":\"album\",");
                struct t_album *album = (struct t_album *)current->user_data;
                buffer = print_album_tags(buffer, &mympd_worker_state->partition_state->mpd_state->config->albums,
                    &mympd_worker_state->partition_state->mpd_state->tags_album, album);
                buffer = sdscatlen(buffer, "}", 1);
                current = current->next;
                if (current != NULL) {
                    buffer = sdscatlen(buffer, ",", 1);
                }
            }
            buffer = sdscatlen(buffer, "],", 2);
            buffer = tojson_uint64(buffer, "totalEntities", add_list.length, true);
            buffer = tojson_uint(buffer, "returnedEntities", add_list.length, false);
            buffer = jsonrpc_end(buffer);
            cache_release_lock(mympd_worker_state->album_cache);
        }
    }
    else if (mode == JUKEBOX_ADD_SONG){
        random_select_songs(mympd_worker_state->partition_state, mympd_worker_state->stickerdb,
            quantity, plist, NULL, &add_list, &constraints);
        buffer = jsonrpc_respond_start(buffer, MYMPD_API_DATABASE_LIST_RANDOM, request_id);
        buffer = sdscat(buffer, "\"data\":[");
        struct t_list_node *current = add_list.head;
        while (current != NULL) {
            if (mpd_send_list_meta(mympd_worker_state->partition_state->conn, current->key)) {
                struct mpd_song *song;
                if ((song = mpd_recv_song(mympd_worker_state->partition_state->conn)) != NULL) {
                    buffer = sdscat(buffer, "{\"Type\":\"song\",");
                    buffer = print_song_tags(buffer, mympd_worker_state->partition_state->mpd_state,
                        &mympd_worker_state->partition_state->mpd_state->tags_mympd, song);
                    buffer = sdscatlen(buffer, "}", 1);
                    mpd_song_free(song);
                }
            }
            mympd_check_error_and_recover(mympd_worker_state->partition_state, NULL, "mpd_send_list_meta");
            current = current->next;
            if (current != NULL) {
                buffer = sdscatlen(buffer, ",", 1);
            }
        }
        buffer = sdscatlen(buffer, "],", 2);
        buffer = tojson_uint64(buffer, "totalEntities", add_list.length, true);
        buffer = tojson_uint(buffer, "returnedEntities", add_list.length, false);
        buffer = jsonrpc_end(buffer);
    }
    else {
        buffer = jsonrpc_respond_message(buffer, MYMPD_API_DATABASE_LIST_RANDOM, request_id,
            JSONRPC_FACILITY_DATABASE, JSONRPC_SEVERITY_ERROR, "Invalid jukebox mode");
    }
    list_clear(&add_list);

    FREE_SDS(error);
    return buffer;
}
