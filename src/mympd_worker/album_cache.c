/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2025 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

/*! \file
 * \brief Album cache creation
 */

#include "compile_time.h"
#include "src/mympd_worker/album_cache.h"

#include "dist/libmympdclient/include/mpd/client.h"
#include "dist/libmympdclient/src/isong.h"
#include "src/lib/cache_rax_album.h"
#include "src/lib/datetime.h"
#include "src/lib/filehandler.h"
#include "src/lib/jsonrpc.h"
#include "src/lib/log.h"
#include "src/lib/msg_queue.h"
#include "src/lib/sds_extras.h"
#include "src/lib/utility.h"
#include "src/mympd_client/errorhandler.h"
#include "src/mympd_client/search.h"
#include "src/mympd_client/tags.h"

#include <inttypes.h>
#include <stdbool.h>
#include <string.h>

/**
 * Private definitions
 */
static bool album_cache_create(struct t_mympd_worker_state *mympd_worker_state, rax *album_cache);
static bool album_cache_create_simple(struct t_mympd_worker_state *mympd_worker_state, rax *album_cache);

/**
 * Public functions
 */

/**
 * Creates the album cache and returns it to mympd_api thread
 * @param mympd_worker_state pointer to mympd_worker_state struct
 * @param force true=force update, false=update only if mpd database is newer then the caches
 * @return true on success else false
 */
bool mympd_worker_album_cache_create(struct t_mympd_worker_state *mympd_worker_state, bool force) {
    time_t db_mtime = mympd_client_get_db_mtime(mympd_worker_state->partition_state);
    sds filepath = sdscatfmt(sdsempty(), "%S/%s/%s", mympd_worker_state->config->workdir, DIR_WORK_TAGS, FILENAME_ALBUMCACHE);
    time_t album_cache_mtime = get_mtime(filepath);
    #ifdef MYMPD_DEBUG
        char fmt_time_db[32];
        readable_time(fmt_time_db, db_mtime);
        char fmt_time_album_cache[32];
        readable_time(fmt_time_album_cache, album_cache_mtime);
        MYMPD_LOG_DEBUG("default", "Database mtime: %s", fmt_time_db);
        MYMPD_LOG_DEBUG("default", "Album cache mtime: %s", fmt_time_album_cache);
    #endif
    FREE_SDS(filepath);

    if (force == false &&
        db_mtime < album_cache_mtime)
    {
        MYMPD_LOG_INFO("default", "Caches are up-to-date");
        send_jsonrpc_notify(JSONRPC_FACILITY_DATABASE, JSONRPC_SEVERITY_INFO, MPD_PARTITION_ALL, "Caches are up-to-date");
        if (mympd_worker_state->partition_state->mpd_state->feat.tags == true) {
            struct t_work_request *request = create_request(REQUEST_TYPE_DISCARD, 0, 0, INTERNAL_API_ALBUMCACHE_SKIPPED, NULL, mympd_worker_state->partition_state->name);
            request->data = jsonrpc_end(request->data);
            mympd_queue_push(mympd_api_queue, request, 0);
        }
        return true;
    }
    send_jsonrpc_event(JSONRPC_EVENT_UPDATE_CACHE_STARTED, MPD_PARTITION_ALL);

    bool rc = true;
    if (mympd_worker_state->partition_state->mpd_state->feat.tags == true) {
        struct t_cache album_cache;
        album_cache.cache = raxNew();
        rc = mympd_worker_state->config->albums.mode == ALBUM_MODE_ADV
            ? album_cache_create(mympd_worker_state, album_cache.cache)
            : album_cache_create_simple(mympd_worker_state, album_cache.cache);
        if (rc == true) {
            struct t_work_request *request = create_request(REQUEST_TYPE_DISCARD, 0, 0, INTERNAL_API_ALBUMCACHE_CREATED, NULL, mympd_worker_state->partition_state->name);
            request->data = jsonrpc_end(request->data);
            request->extra = (void *) album_cache.cache;
            mympd_queue_push(mympd_api_queue, request, 0);
            send_jsonrpc_notify(JSONRPC_FACILITY_DATABASE, JSONRPC_SEVERITY_INFO, MPD_PARTITION_ALL, "Updated album cache");
            if (mympd_worker_state->config->save_caches == true) {
                album_cache_write(&album_cache, mympd_worker_state->config->workdir,
                    &mympd_worker_state->mpd_state->tags_album, &mympd_worker_state->config->albums, false);
            }
        }
        else {
            album_cache_free(&album_cache);
            send_jsonrpc_notify(JSONRPC_FACILITY_DATABASE, JSONRPC_SEVERITY_ERROR, MPD_PARTITION_ALL, "Update of album cache failed");
            struct t_work_request *request = create_request(REQUEST_TYPE_DISCARD, 0, 0, INTERNAL_API_ALBUMCACHE_ERROR, NULL, mympd_worker_state->partition_state->name);
            request->data = jsonrpc_end(request->data);
            mympd_queue_push(mympd_api_queue, request, 0);
        }
    }
    else {
        MYMPD_LOG_INFO("default", "Skipped album cache creation, tags are disabled");
        struct t_work_request *request = create_request(REQUEST_TYPE_DISCARD, 0, 0, INTERNAL_API_ALBUMCACHE_SKIPPED, NULL, mympd_worker_state->partition_state->name);
        request->data = jsonrpc_end(request->data);
        mympd_queue_push(mympd_api_queue, request, 0);
    }

    send_jsonrpc_event(JSONRPC_EVENT_UPDATE_CACHE_FINISHED, MPD_PARTITION_ALL);
    return rc;
}

/**
 * Private functions
 */

/**
 * Initializes the album cache
 * @param mympd_worker_state pointer to mympd_worker_state struct
 * @param album_cache pointer to empty album_cache
 * @return true on success, else false
 */
static bool album_cache_create(struct t_mympd_worker_state *mympd_worker_state, rax *album_cache) {
    MYMPD_LOG_INFO("default", "Creating album cache");
    if (mympd_worker_state->config->albums.group_tag != MPD_TAG_UNKNOWN) {
        MYMPD_LOG_DEBUG("default", "Additional group tag: %s", mpd_tag_name(mympd_worker_state->config->albums.group_tag));
    }
    else {
        MYMPD_LOG_DEBUG("default", "Additional group tag: None");
    }

    unsigned start = 0;
    unsigned end = start + MPD_RESULTS_MAX;
    unsigned i = 0;
    int album_count = 0;
    int skip_count = 0;

    //set interesting tags
    if (mympd_client_tag_exists(&mympd_worker_state->mpd_state->tags_mympd, MPD_TAG_DISC) == true) {
        if (mympd_client_tag_exists(&mympd_worker_state->mpd_state->tags_album, MPD_TAG_DISC) == false) {
            mympd_worker_state->mpd_state->tags_album.tags[mympd_worker_state->mpd_state->tags_album.len++] = MPD_TAG_DISC;
        }
    }
    else {
        MYMPD_LOG_WARN("default", "Disc tag is not enabled");
    }
    if (mympd_worker_state->config->albums.group_tag != MPD_TAG_UNKNOWN) {
        if (mympd_client_tag_exists(&mympd_worker_state->mpd_state->tags_mympd, mympd_worker_state->config->albums.group_tag) == true) {
            if (mympd_client_tag_exists(&mympd_worker_state->mpd_state->tags_album, mympd_worker_state->config->albums.group_tag) == false) {
                mympd_worker_state->mpd_state->tags_album.tags[mympd_worker_state->mpd_state->tags_album.len++] = mympd_worker_state->config->albums.group_tag;
            }
        }
        else {
            MYMPD_LOG_WARN("default", "%s tag is not enabled", mpd_tag_name(mympd_worker_state->config->albums.group_tag));
        }
    }
    enable_mpd_tags(mympd_worker_state->partition_state, &mympd_worker_state->mpd_state->tags_album);

    //get all songs and set albums
    #ifdef MYMPD_DEBUG
        MEASURE_INIT
        MEASURE_START
    #endif
    sds key = sdsempty();
    do {
        if (mpd_search_db_songs(mympd_worker_state->partition_state->conn, false) == false ||
            mpd_search_add_expression(mympd_worker_state->partition_state->conn, "((Album != '') AND (AlbumArtist !=''))") == false ||
            mpd_search_add_window(mympd_worker_state->partition_state->conn, start, end) == false)
        {
            MYMPD_LOG_ERROR("default", "Cache update failed");
            mpd_search_cancel(mympd_worker_state->partition_state->conn);
            return false;
        }
        if (mpd_search_commit(mympd_worker_state->partition_state->conn)) {
            struct mpd_song *song;
            while ((song = mpd_recv_song(mympd_worker_state->partition_state->conn)) != NULL) {
                // set initial song and disc count to 1
                album_cache_set_song_count(song, 1);
                if (mympd_worker_state->tag_disc_empty_is_first == true) {
                    // handle empty disc tag as disc one
                    album_cache_set_disc_count(song, 1);
                }
                // construct the key
                key = album_cache_get_key(key, song, &mympd_worker_state->config->albums);
                if (sdslen(key) > 0) {
                    if (mympd_worker_state->partition_state->mpd_state->tag_albumartist == MPD_TAG_ALBUM_ARTIST &&
                        mpd_song_get_tag(song, MPD_TAG_ALBUM_ARTIST, 0) == NULL)
                    {
                        // Copy Artist tag to AlbumArtist tag
                        // for filters mpd falls back from AlbumArtist to Artist if AlbumArtist does not exist
                        album_cache_copy_tags(song, MPD_TAG_ARTIST, MPD_TAG_ALBUM_ARTIST);
                    }
                    void *old_data;
                    if (raxTryInsert(album_cache, (unsigned char *)key, sdslen(key), (void *)song, &old_data) == 0) {
                        // existing album: append song data
                        struct mpd_song *album = (struct mpd_song *) old_data;
                        // append tags
                        album_cache_append_tags(album, song, &mympd_worker_state->partition_state->mpd_state->tags_mympd);
                        // set album data
                        album_cache_set_last_modified(album, song); // use latest last_modified
                        album_cache_inc_total_time(album, song);    // sum duration
                        album_cache_set_discs(album, song);         // use max disc value
                        album_cache_inc_song_count(album);          // inc song count by one
                        // free song data
                        mpd_song_free(song);
                    }
                    else {
                        // new album: use song data as initial album data
                        album_count++;
                    }
                }
                else {
                    skip_count++;
                    mpd_song_free(song);
                }
                i++;
            }
        }
        mpd_response_finish(mympd_worker_state->partition_state->conn);
        if (mympd_check_error_and_recover(mympd_worker_state->partition_state, NULL, "mpd_search_commit") == false) {
            MYMPD_LOG_ERROR("default", "Cache update failed");
            return false;
        }
        start = end;
        end = end + MPD_RESULTS_MAX;
    } while (i >= start);
    FREE_SDS(key);
    #ifdef MYMPD_DEBUG
        MEASURE_END
        MEASURE_PRINT("default", "Populate album cache")
    #endif

    //finished - print statistics
    MYMPD_LOG_INFO("default", "Added %d albums to album cache", album_count);
    if (skip_count > 0) {
        MYMPD_LOG_WARN("default", "Skipped %d songs for album cache", skip_count);
    }
    MYMPD_LOG_INFO("default", "Cache updated successfully");
    return true;
}

/**
 * Initializes the simple album cache.
 * This is faster as the cache_init function, but does not fetch all the album details.
 * @param mympd_worker_state pointer to mympd_worker_state struct
 * @param album_cache pointer to empty album_cache
 * @return true on success, else false
 */
static bool album_cache_create_simple(struct t_mympd_worker_state *mympd_worker_state, rax *album_cache) {
    MYMPD_LOG_INFO("default", "Creating simple album cache");
    if (mympd_worker_state->config->albums.group_tag != MPD_TAG_UNKNOWN) {
        MYMPD_LOG_DEBUG("default", "Additional group tag: %s", mpd_tag_name(mympd_worker_state->config->albums.group_tag));
    }
    else {
        MYMPD_LOG_DEBUG("default", "Additional group tag: None");
    }
    int album_count = 0;
    int skip_count = 0;

    //check for required tags
    if (mympd_client_tag_exists(&mympd_worker_state->mpd_state->tags_mympd, MPD_TAG_ARTIST) == false ||
        mympd_client_tag_exists(&mympd_worker_state->mpd_state->tags_mympd, MPD_TAG_ALBUM) == false)
    {
        MYMPD_LOG_ERROR("default", "Required tags for album cache creation are not enabled: Artist, Album");
        return false;
    }
    if (mympd_worker_state->config->albums.group_tag != MPD_TAG_UNKNOWN) {
        if (mympd_client_tag_exists(&mympd_worker_state->mpd_state->tags_mympd, mympd_worker_state->config->albums.group_tag) == false) {
            MYMPD_LOG_ERROR("default", "Required tag for album cache creation is not enabled: %s", mpd_tag_name(mympd_worker_state->config->albums.group_tag));
        return false;
        }
    }

    //get all songs and set albums
    #ifdef MYMPD_DEBUG
        MEASURE_INIT
        MEASURE_START
    #endif
    sds key = sdsempty();
    sds album = sdsempty();
    sds artist = sdsempty();
    sds group_tag = sdsempty();
    if (mpd_search_db_tags(mympd_worker_state->partition_state->conn, MPD_TAG_ALBUM) == false ||
        mpd_search_add_group_tag(mympd_worker_state->partition_state->conn, MPD_TAG_ALBUM_ARTIST) == false ||
        mympd_client_add_search_group_param(mympd_worker_state->partition_state->conn, mympd_worker_state->config->albums.group_tag) == false)
    {
        MYMPD_LOG_ERROR("default", "Cache update failed");
        mpd_search_cancel(mympd_worker_state->partition_state->conn);
        return false;
    }
    if (mpd_search_commit(mympd_worker_state->partition_state->conn)) {
        struct mpd_pair *pair;
        while ((pair = mpd_recv_pair(mympd_worker_state->partition_state->conn)) != NULL) {
            if (strcmp(pair->name, mpd_tag_name(MPD_TAG_ALBUM)) == 0) {
                album = sds_replace(album, pair->value);
                if (sdslen(album) > 0 &&
                    sdslen(artist) > 0)
                {
                    // we do not fetch the uri for performance reasons.
                    // the real song uri will be set after first call of mympd_api_albumart_getcover_by_album_id.
                    struct mpd_song *song = mpd_song_new("albumid");
                    mympd_mpd_song_add_tag_dedup(song, MPD_TAG_ARTIST, artist);
                    mympd_mpd_song_add_tag_dedup(song, MPD_TAG_ALBUM_ARTIST, artist);
                    mympd_mpd_song_add_tag_dedup(song, MPD_TAG_ALBUM, album);
                    if (sdslen(group_tag) > 0) {
                        mympd_mpd_song_add_tag_dedup(song, mympd_worker_state->config->albums.group_tag, group_tag);
                    }
                    // insert album into cache
                    key = album_cache_get_key(key, song, &mympd_worker_state->config->albums);
                    raxInsert(album_cache, (unsigned char *)key, sdslen(key), (void *)song, NULL);
                    album_count++;
                    sdsclear(artist);
                    sdsclear(album);
                    sdsclear(group_tag);
                }
                else {
                    skip_count++;
                }
            }
            else if (strcmp(pair->name, mpd_tag_name(MPD_TAG_ALBUM_ARTIST)) == 0) {
                artist = sds_replace(artist, pair->value);
            }
            else if (strcmp(pair->name, mpd_tag_name(mympd_worker_state->config->albums.group_tag)) == 0) {
                group_tag = sds_replace(group_tag, pair->value);
            }
            mpd_return_pair(mympd_worker_state->partition_state->conn, pair);
        }
    }
    mpd_response_finish(mympd_worker_state->partition_state->conn);
    if (mympd_check_error_and_recover(mympd_worker_state->partition_state, NULL, "mpd_search_commit") == false) {
        MYMPD_LOG_ERROR("default", "Cache update failed");
        return false;
    }
    FREE_SDS(key);
    FREE_SDS(album);
    FREE_SDS(artist);
    FREE_SDS(group_tag);
    #ifdef MYMPD_DEBUG
        MEASURE_END
        MEASURE_PRINT("default", "Populate album cache")
    #endif

    //finished - print statistics
    MYMPD_LOG_INFO("default", "Added %d albums to album cache", album_count);
    if (skip_count > 0) {
        MYMPD_LOG_WARN("default", "Skipped %d albums for album cache", skip_count);
    }
    MYMPD_LOG_INFO("default", "Cache updated successfully");
    return true;
}
