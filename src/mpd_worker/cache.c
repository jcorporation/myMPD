/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2022 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include "compile_time.h"
#include "cache.h"

#include "../lib/album_cache.h"
#include "../lib/jsonrpc.h"
#include "../lib/log.h"
#include "../lib/mem.h"
#include "../lib/msg_queue.h"
#include "../lib/sds_extras.h"
#include "../lib/sticker_cache.h"
#include "../mpd_client/errorhandler.h"
#include "../mpd_client/tags.h"

#include <inttypes.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/**
 * Privat definitions
 */
static bool _cache_init(struct t_mpd_worker_state *mpd_worker_state, rax *album_cache, rax *sticker_cache);
static bool _get_sticker_from_mpd(struct t_partition_state *partition_state, const char *uri, struct t_sticker *sticker);

/**
 * Public functions
 */

/**
 * Creates the caches and returns it to mympd_api thread
 * @param mpd_worker_state pointer to mpd_worker_state struct
 * @return true on success else false
 */
bool mpd_worker_cache_init(struct t_mpd_worker_state *mpd_worker_state) {
    struct t_cache album_cache;
    album_cache.cache = NULL;
    if (mpd_worker_state->partition_state->mpd_state->feat_tags == true) {
        album_cache.cache = raxNew();
    }
    struct t_cache sticker_cache;
    sticker_cache.cache = NULL;
    if (mpd_worker_state->partition_state->mpd_state->feat_stickers == true) {
        sticker_cache.cache = raxNew();
    }

    bool rc = true;
    if (mpd_worker_state->partition_state->mpd_state->feat_tags == true ||
        mpd_worker_state->partition_state->mpd_state->feat_stickers == true)
    {
        rc =_cache_init(mpd_worker_state, album_cache.cache, sticker_cache.cache);
    }

    //push album cache building response to mpd_client thread
    if (mpd_worker_state->partition_state->mpd_state->feat_tags == true) {
        if (rc == true) {
            struct t_work_request *request = create_request(-1, 0, INTERNAL_API_ALBUMCACHE_CREATED, NULL, mpd_worker_state->partition_state->name);
            request->data = jsonrpc_end(request->data);
            request->extra = (void *) album_cache.cache;
            mympd_queue_push(mympd_api_queue, request, 0);
            send_jsonrpc_notify(JSONRPC_FACILITY_DATABASE, JSONRPC_SEVERITY_INFO, mpd_worker_state->partition_state->name, "Updated album cache");
        }
        else {
            album_cache_free(&album_cache);
            send_jsonrpc_notify(JSONRPC_FACILITY_DATABASE, JSONRPC_SEVERITY_ERROR, mpd_worker_state->partition_state->name, "Update of album cache failed");
        }
    }
    else {
        MYMPD_LOG_INFO("Skipped album cache creation, tags are disabled");
    }

    //push sticker cache building response to mpd_client thread
    if (mpd_worker_state->partition_state->mpd_state->feat_stickers == true) {
        if (rc == true) {
            struct t_work_request *request = create_request(-1, 0, INTERNAL_API_STICKERCACHE_CREATED, NULL, mpd_worker_state->partition_state->name);
            request->data = jsonrpc_end(request->data);
            request->extra = (void *) sticker_cache.cache;
            mympd_queue_push(mympd_api_queue, request, 0);
            send_jsonrpc_notify(JSONRPC_FACILITY_DATABASE, JSONRPC_SEVERITY_INFO, mpd_worker_state->partition_state->name, "Updated sticker cache");
        }
        else {
            sticker_cache_free(&sticker_cache);
            send_jsonrpc_notify(JSONRPC_FACILITY_DATABASE, JSONRPC_SEVERITY_ERROR, mpd_worker_state->partition_state->name, "Update of sticker cache failed");
        }
    }
    else {
        MYMPD_LOG_INFO("Skipped sticker cache creation, stickers are disabled");
    }
    return rc;
}

/**
 * Private functions
 */

/**
 * Initializes the album and sticker cache
 * @param mpd_worker_state pointer to mpd_worker_state struct
 * @param album_cache pointer to empty album_cache
 * @param sticker_cache sticker_cache pointer to empty sticker_cache
 * @return true on success else false
 */
static bool _cache_init(struct t_mpd_worker_state *mpd_worker_state, rax *album_cache, rax *sticker_cache) {
    MYMPD_LOG_INFO("Creating caches");
    unsigned start = 0;
    unsigned end = start + MPD_RESULTS_MAX;
    unsigned i = 0;
    long album_count = 0;
    long song_count = 0;
    long skipped = 0;
    //get first song from each album
    do {
        bool rc = mpd_search_db_songs(mpd_worker_state->partition_state->conn, false);
        if (mympd_check_rc_error_and_recover(mpd_worker_state->partition_state, rc, "mpd_search_db_songs") == false) {
            MYMPD_LOG_ERROR("Cache update failed");
            mpd_search_cancel(mpd_worker_state->partition_state->conn);
            return false;
        }
        rc = mpd_search_add_uri_constraint(mpd_worker_state->partition_state->conn, MPD_OPERATOR_DEFAULT, "");
        if (mympd_check_rc_error_and_recover(mpd_worker_state->partition_state, rc, "mpd_search_add_uri_constraint") == false) {
            MYMPD_LOG_ERROR("Cache update failed");
            mpd_search_cancel(mpd_worker_state->partition_state->conn);
            return false;
        }
        rc = mpd_search_add_window(mpd_worker_state->partition_state->conn, start, end);
        if (mympd_check_rc_error_and_recover(mpd_worker_state->partition_state, rc, "mpd_search_add_window") == false) {
            MYMPD_LOG_ERROR("Cache update failed");
            mpd_search_cancel(mpd_worker_state->partition_state->conn);
            return false;
        }
        rc = mpd_search_commit(mpd_worker_state->partition_state->conn);
        if (mympd_check_rc_error_and_recover(mpd_worker_state->partition_state, rc, "mpd_search_commit") == false) {
            MYMPD_LOG_ERROR("Cache update failed");
            return false;
        }
        struct mpd_song *song;
        sds key = sdsempty();
        const bool create_album_cache = mpd_worker_state->partition_state->mpd_state->feat_tags &&
            mpd_client_tag_exists(&mpd_worker_state->partition_state->mpd_state->tags_mympd, MPD_TAG_ALBUM) &&
            mpd_client_tag_exists(&mpd_worker_state->partition_state->mpd_state->tags_mympd, mpd_worker_state->partition_state->mpd_state->tag_albumartist);
        if (create_album_cache == false) {
            MYMPD_LOG_NOTICE("Skipping album cache creation, (Album)Artist and Album tags must be enabled");
        }
        while ((song = mpd_recv_song(mpd_worker_state->partition_state->conn)) != NULL) {
            //sticker cache
            if (mpd_worker_state->partition_state->mpd_state->feat_stickers == true) {
                const char *uri = mpd_song_get_uri(song);
                struct t_sticker *sticker = malloc_assert(sizeof(struct t_sticker));
                if (raxTryInsert(sticker_cache, (unsigned char *)uri, strlen(uri), (void *)sticker, NULL) == 0) {
                    MYMPD_LOG_ERROR("Error adding \"%s\" to sticker cache", uri);
                    FREE_PTR(sticker);
                }
                else {
                    song_count++;
                }
            }
            //album cache
            if (create_album_cache == true) {
                //set initial soung count to 1
                album_cache_set_song_count(song, 1);
                //construct the key
                key = album_cache_get_key(song, key);
                if (sdslen(key) > 0) {
                    if (mpd_worker_state->partition_state->mpd_state->tag_albumartist == MPD_TAG_ALBUM_ARTIST &&
                        mpd_song_get_tag(song, MPD_TAG_ALBUM_ARTIST, 0) == NULL)
                    {
                        //Copy Artist tag to AlbumArtist tag
                        //for filters mpd falls back from AlbumArtist to Artist if AlbumArtist does not exist
                        album_cache_copy_tags(song, MPD_TAG_ARTIST, MPD_TAG_ALBUM_ARTIST);
                    }
                    void *old_data;
                    if (raxTryInsert(album_cache, (unsigned char *)key, sdslen(key), (void *)song, &old_data) == 0) {
                        struct mpd_song *album = (struct mpd_song *) old_data;
                        //append song data if key exists
                        album_cache_append_tags(album, song, &mpd_worker_state->partition_state->mpd_state->tags_mympd);
                        //set album data
                        album_cache_set_last_modified(album, song); //use latest last_modified
                        album_cache_inc_total_time(album, song);    //sum duration
                        album_cache_set_discs(album, song);         //use max disc value
                        album_cache_inc_song_count(album);          //inc song count by one
                        //free song data
                        mpd_song_free(song);
                    }
                    else {
                        album_count++;
                    }
                }
                else {
                    skipped++;
                    mpd_song_free(song);
                }
            }
            i++;
        }
        FREE_SDS(key);
        mpd_response_finish(mpd_worker_state->partition_state->conn);
        if (mympd_check_error_and_recover(mpd_worker_state->partition_state) == false) {
            MYMPD_LOG_ERROR("Cache update failed");
            return false;
        }
        start = end;
        end = end + MPD_RESULTS_MAX;
    } while (i >= start);
    //get sticker values
    if (mpd_worker_state->partition_state->mpd_state->feat_stickers == true) {
        raxIterator iter;
        raxStart(&iter, sticker_cache);
        raxSeek(&iter, "^", NULL, 0);
        sds uri = sdsempty();
        while (raxNext(&iter)) {
            uri = sds_replacelen(uri, (char *)iter.key, iter.key_len);
            _get_sticker_from_mpd(mpd_worker_state->partition_state, uri, (struct t_sticker *)iter.data);
        }
        FREE_SDS(uri);
        raxStop(&iter);
    }
    MYMPD_LOG_INFO("Added %ld albums to album cache", album_count);
    if (skipped > 0) {
        MYMPD_LOG_WARN("Skipped %ld songs for album cache", skipped);
    }
    MYMPD_LOG_INFO("Added %ld songs to sticker cache", song_count);
    MYMPD_LOG_INFO("Cache updated successfully");
    return true;
}

/**
 * Populates the sticker struct from mpd
 * @param partition_state pointer to partition specific states
 * @param uri song uri
 * @param sticker pointer already allocated sticker struct to populate
 * @return true on success else false
 */
static bool _get_sticker_from_mpd(struct t_partition_state *partition_state, const char *uri, struct t_sticker *sticker) {
    struct mpd_pair *pair;
    char *crap = NULL;
    sticker->play_count = 0;
    sticker->skip_count = 0;
    sticker->last_played = 0;
    sticker->last_skipped = 0;
    sticker->like = 1;

    bool rc = mpd_send_sticker_list(partition_state->conn, "song", uri);
    if (mympd_check_rc_error_and_recover(partition_state, rc, "mpd_send_sticker_list") == false) {
        return false;
    }

    while ((pair = mpd_recv_sticker(partition_state->conn)) != NULL) {
        if (strcmp(pair->name, "playCount") == 0) {
            sticker->play_count = (long)strtoimax(pair->value, &crap, 10);
        }
        else if (strcmp(pair->name, "skipCount") == 0) {
            sticker->skip_count = (long)strtoimax(pair->value, &crap, 10);
        }
        else if (strcmp(pair->name, "lastPlayed") == 0) {
            sticker->last_played = (time_t)strtoimax(pair->value, &crap, 10);
        }
        else if (strcmp(pair->name, "lastSkipped") == 0) {
            sticker->last_skipped = (time_t)strtoimax(pair->value, &crap, 10);
        }
        else if (strcmp(pair->name, "like") == 0) {
            sticker->like = (int)strtoimax(pair->value, &crap, 10);
        }
        mpd_return_sticker(partition_state->conn, pair);
    }
    mpd_response_finish(partition_state->conn);
    if (mympd_check_error_and_recover(partition_state) == false) {
        return false;
    }

    return true;
}
