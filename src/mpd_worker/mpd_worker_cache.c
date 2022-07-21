/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2022 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include "mympd_config_defs.h"
#include "mpd_worker_cache.h"

#include "../lib/album_cache.h"
#include "../lib/jsonrpc.h"
#include "../lib/log.h"
#include "../lib/mem.h"
#include "../lib/sds_extras.h"
#include "../lib/sticker_cache.h"
#include "../mpd_client/mpd_client_errorhandler.h"
#include "../mpd_client/mpd_client_tags.h"

#include <inttypes.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

//privat definitions
static bool _cache_init(struct t_mpd_worker_state *mpd_worker_state, rax *album_cache, rax *sticker_cache);
static bool _get_sticker_from_mpd(struct t_mpd_state *mpd_state, const char *uri, struct t_sticker *sticker);

//public functions

/**
 * Creates the caches and returns it to mympd_api thread
 * @param mpd_worker_state pointer to mpd_worker_state struct
 * @return true on success else false
 */
bool mpd_worker_cache_init(struct t_mpd_worker_state *mpd_worker_state) {
    struct t_cache album_cache;
    album_cache.cache = NULL;
    if (mpd_worker_state->mpd_state->feat_mpd_tags == true) {
        album_cache.cache = raxNew();
    }
    struct t_cache sticker_cache;
    sticker_cache.cache = NULL;
    if (mpd_worker_state->mpd_state->feat_mpd_stickers == true) {
        sticker_cache.cache = raxNew();
    }

    bool rc = true;
    if (mpd_worker_state->mpd_state->feat_mpd_tags == true ||
        mpd_worker_state->mpd_state->feat_mpd_stickers == true)
    {
        rc =_cache_init(mpd_worker_state, album_cache.cache, sticker_cache.cache);
    }

    //push album cache building response to mpd_client thread
    if (mpd_worker_state->mpd_state->feat_mpd_tags == true) {
        if (rc == true) {
            struct t_work_request *request = create_request(-1, 0, INTERNAL_API_ALBUMCACHE_CREATED, NULL);
            request->data = sdscatlen(request->data, "}}", 2);
            request->extra = (void *) album_cache.cache;
            mympd_queue_push(mympd_api_queue, request, 0);
            send_jsonrpc_notify(JSONRPC_FACILITY_DATABASE, JSONRPC_SEVERITY_INFO, "Updated album cache");
        }
        else {
            album_cache_free(&album_cache);
            send_jsonrpc_notify(JSONRPC_FACILITY_DATABASE, JSONRPC_SEVERITY_ERROR, "Update of album cache failed");
        }
    }
    else {
        MYMPD_LOG_INFO("Skipped album cache creation, tags are disabled");
    }

    //push sticker cache building response to mpd_client thread
    if (mpd_worker_state->mpd_state->feat_mpd_stickers == true) {
        if (rc == true) {
            struct t_work_request *request = create_request(-1, 0, INTERNAL_API_STICKERCACHE_CREATED, NULL);
            request->data = sdscatlen(request->data, "}}", 2);
            request->extra = (void *) sticker_cache.cache;
            mympd_queue_push(mympd_api_queue, request, 0);
            send_jsonrpc_notify(JSONRPC_FACILITY_DATABASE, JSONRPC_SEVERITY_INFO, "Updated sticker cache");
        }
        else {
            sticker_cache_free(&sticker_cache);
            send_jsonrpc_notify(JSONRPC_FACILITY_DATABASE, JSONRPC_SEVERITY_ERROR, "Update of sticker cache failed");
        }
    }
    else {
        MYMPD_LOG_INFO("Skipped sticker cache creation, stickers are disabled");
    }
    return rc;
}

//private functions

/**
 * Initializes the album and sticker cache
 * @param mpd_worker_state pointer to mpd_worker_state struct
 * @album_cache pointer to empty album_cache
 * @album sticker_cache pointer to empty sticker_cache
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
        bool rc = mpd_search_db_songs(mpd_worker_state->mpd_state->conn, false);
        if (mympd_check_rc_error_and_recover(mpd_worker_state->mpd_state, rc, "mpd_search_db_songs") == false) {
            MYMPD_LOG_ERROR("Cache update failed");
            mpd_search_cancel(mpd_worker_state->mpd_state->conn);
            return false;
        }
        rc = mpd_search_add_uri_constraint(mpd_worker_state->mpd_state->conn, MPD_OPERATOR_DEFAULT, "");
        if (mympd_check_rc_error_and_recover(mpd_worker_state->mpd_state, rc, "mpd_search_add_uri_constraint") == false) {
            MYMPD_LOG_ERROR("Cache update failed");
            mpd_search_cancel(mpd_worker_state->mpd_state->conn);
            return false;
        }
        rc = mpd_search_add_window(mpd_worker_state->mpd_state->conn, start, end);
        if (mympd_check_rc_error_and_recover(mpd_worker_state->mpd_state, rc, "mpd_search_add_window") == false) {
            MYMPD_LOG_ERROR("Cache update failed");
            mpd_search_cancel(mpd_worker_state->mpd_state->conn);
            return false;
        }
        rc = mpd_search_commit(mpd_worker_state->mpd_state->conn);
        if (mympd_check_rc_error_and_recover(mpd_worker_state->mpd_state, rc, "mpd_search_commit") == false) {
            MYMPD_LOG_ERROR("Cache update failed");
            return false;
        }
        struct mpd_song *song;
        sds key = sdsempty();
        const bool create_album_cache = mpd_worker_state->mpd_state->feat_mpd_tags &&
            mpd_client_tag_exists(&mpd_worker_state->mpd_state->tag_types_mympd, MPD_TAG_ALBUM) &&
            mpd_client_tag_exists(&mpd_worker_state->mpd_state->tag_types_mympd, mpd_worker_state->mpd_state->tag_albumartist);
        if (create_album_cache == false) {
            MYMPD_LOG_NOTICE("Skipping album cache creation, (Album)Artist and Album tags must be enabled");
        }
        while ((song = mpd_recv_song(mpd_worker_state->mpd_state->conn)) != NULL) {
            //sticker cache
            if (mpd_worker_state->mpd_state->feat_mpd_stickers == true) {
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
                key = album_cache_get_key(song, key, mpd_worker_state->mpd_state->tag_albumartist);
                if (sdslen(key) > 0) {
                    void *old_data;
                    if (raxTryInsert(album_cache, (unsigned char *)key, sdslen(key), (void *)song, &old_data) == 0) {
                        struct mpd_song *album = (struct mpd_song *) old_data;
                        //append song data if key exists
                        album_cache_append_tags(album, song, &mpd_worker_state->mpd_state->tag_types_mympd);
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
                    MYMPD_LOG_DEBUG("Albumcache, skipping \"%s\"", mpd_song_get_uri(song));
                    skipped++;
                    mpd_song_free(song);
                }
            }
            i++;
        }
        FREE_SDS(key);
        mpd_response_finish(mpd_worker_state->mpd_state->conn);
        if (mympd_check_error_and_recover(mpd_worker_state->mpd_state) == false) {
            MYMPD_LOG_ERROR("Cache update failed");
            return false;
        }
        start = end;
        end = end + MPD_RESULTS_MAX;
    } while (i >= start);
    //get sticker values
    if (mpd_worker_state->mpd_state->feat_mpd_stickers == true) {
        raxIterator iter;
        raxStart(&iter, sticker_cache);
        raxSeek(&iter, "^", NULL, 0);
        sds uri = sdsempty();
        while (raxNext(&iter)) {
            uri = sds_replacelen(uri, (char *)iter.key, iter.key_len);
            _get_sticker_from_mpd(mpd_worker_state->mpd_state, uri, (struct t_sticker *)iter.data);
        }
        FREE_SDS(uri);
        raxStop(&iter);
    }
    MYMPD_LOG_INFO("Added %ld albums to album cache", album_count);
    MYMPD_LOG_INFO("Skipped %ld songs", skipped);
    MYMPD_LOG_INFO("Added %ld songs to sticker cache", song_count);
    MYMPD_LOG_INFO("Cache updated successfully");
    return true;
}

/**
 * Populates the sticker struct from mpd
 * @param mpd_state pointer to t_mpd_state struct
 * @param uri song uri
 * @param pointer already allocated sticker struct to populate
 * @return true on success else false
 */
static bool _get_sticker_from_mpd(struct t_mpd_state *mpd_state, const char *uri, struct t_sticker *sticker) {
    struct mpd_pair *pair;
    char *crap = NULL;
    sticker->playCount = 0;
    sticker->skipCount = 0;
    sticker->lastPlayed = 0;
    sticker->lastSkipped = 0;
    sticker->like = 1;

    bool rc = mpd_send_sticker_list(mpd_state->conn, "song", uri);
    if (mympd_check_rc_error_and_recover(mpd_state, rc, "mpd_send_sticker_list") == false) {
        return false;
    }

    while ((pair = mpd_recv_sticker(mpd_state->conn)) != NULL) {
        if (strcmp(pair->name, "playCount") == 0) {
            sticker->playCount = (long)strtoimax(pair->value, &crap, 10);
        }
        else if (strcmp(pair->name, "skipCount") == 0) {
            sticker->skipCount = (long)strtoimax(pair->value, &crap, 10);
        }
        else if (strcmp(pair->name, "lastPlayed") == 0) {
            sticker->lastPlayed = (time_t)strtoimax(pair->value, &crap, 10);
        }
        else if (strcmp(pair->name, "lastSkipped") == 0) {
            sticker->lastSkipped = (time_t)strtoimax(pair->value, &crap, 10);
        }
        else if (strcmp(pair->name, "like") == 0) {
            sticker->like = (int)strtoimax(pair->value, &crap, 10);
        }
        mpd_return_sticker(mpd_state->conn, pair);
    }
    mpd_response_finish(mpd_state->conn);
    if (mympd_check_error_and_recover(mpd_state) == false) {
        return false;
    }

    return true;
}
