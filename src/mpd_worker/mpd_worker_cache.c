/*
 SPDX-License-Identifier: GPL-2.0-or-later
 myMPD (c) 2018-2021 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <inttypes.h>
#include <limits.h>
#include <time.h>
#include <assert.h>
#include <signal.h>
#include <mpd/client.h>

#include "../../dist/src/sds/sds.h"
#include "../sds_extras.h"
#include "../api.h"
#include "../log.h"
#include "../list.h"
#include "config_defs.h"
#include "../utility.h"
#include "../tiny_queue.h"
#include "../global.h"
#include "../mpd_shared/mpd_shared_typedefs.h"
#include "../mpd_shared/mpd_shared_tags.h"
#include "../mpd_shared.h"
#include "../mpd_shared/mpd_shared_sticker.h"
#include "mpd_worker_utility.h"
#include "mpd_worker_cache.h"

//privat definitions
static bool _cache_init(t_mpd_worker_state *mpd_worker_state, rax *album_cache, rax *sticker_cache, bool feat_tags, bool feat_sticker);

//public functions
bool mpd_worker_cache_init(t_mpd_worker_state *mpd_worker_state, bool feat_tags, bool feat_sticker) {
    rax *album_cache = NULL;
    if (feat_tags == true) {
        album_cache = raxNew();
    }
    rax *sticker_cache = NULL;
    if (feat_sticker == true) {
        sticker_cache = raxNew();
    }
    
    bool rc = true;
    if (feat_tags == true || feat_sticker == true) {
        rc =_cache_init(mpd_worker_state, album_cache, sticker_cache, feat_tags, feat_sticker);
    }

    //push album cache building response to mpd_client thread
    if (feat_tags == true) {
        if (rc == true) {
            t_work_request *request = create_request(-1, 0, MPD_API_ALBUMCACHE_CREATED, "MPD_API_ALBUMCACHE_CREATED", "");
            request->data = sdscat(request->data, "{\"jsonrpc\":\"2.0\",\"id\":0,\"method\":\"MPD_API_ALBUMCACHE_CREATED\",\"params\":{}}");
            request->extra = (void *) album_cache;
            tiny_queue_push(mpd_client_queue, request, 0);
            send_jsonrpc_notify("database", "info", "Updated album cache");
        }
        else {
            album_cache_free(&album_cache);
            send_jsonrpc_notify("database", "error", "Update of album cache failed");
        }
    }
    else {
        MYMPD_LOG_INFO("Skipped album cache creation, tags are disabled");
    }

    //push sticker cache building response to mpd_client thread
    if (feat_sticker == true) {
        if (rc == true) {
            t_work_request *request2 = create_request(-1, 0, MPD_API_STICKERCACHE_CREATED, "MPD_API_STICKERCACHE_CREATED", "");
            request2->data = sdscat(request2->data, "{\"jsonrpc\":\"2.0\",\"id\":0,\"method\":\"MPD_API_STICKERCACHE_CREATED\",\"params\":{}}");
            request2->extra = (void *) sticker_cache;
            tiny_queue_push(mpd_client_queue, request2, 0);
            send_jsonrpc_notify("database", "info", "Updated sticker cache");
        }
        else {
            sticker_cache_free(&sticker_cache);
            send_jsonrpc_notify("database", "error", "Update of sticker cache failed");
        }
    }
    else {
        MYMPD_LOG_INFO("Skipped sticker cache creation, stickers are disabled");
    }
    return rc;
}

//private functions
static bool _cache_init(t_mpd_worker_state *mpd_worker_state, rax *album_cache, rax *sticker_cache, bool feat_tags, bool feat_sticker) {
    MYMPD_LOG_INFO("Creating caches");
    unsigned start = 0;
    unsigned end = start + 1000;
    unsigned i = 0;   
    unsigned album_count = 0;
    unsigned song_count = 0;
    //get first song from each album
    do {
        bool rc = mpd_search_db_songs(mpd_worker_state->mpd_state->conn, false);
        if (check_rc_error_and_recover(mpd_worker_state->mpd_state, NULL, NULL, 0, false, rc, "mpd_search_db_songs") == false) {
            MYMPD_LOG_ERROR("Cache update failed");
            mpd_search_cancel(mpd_worker_state->mpd_state->conn);
            return false;
        }
        rc = mpd_search_add_uri_constraint(mpd_worker_state->mpd_state->conn, MPD_OPERATOR_DEFAULT, "");
        if (check_rc_error_and_recover(mpd_worker_state->mpd_state, NULL, NULL, 0, false, rc, "mpd_search_add_uri_constraint") == false) {
            MYMPD_LOG_ERROR("Cache update failed");
            mpd_search_cancel(mpd_worker_state->mpd_state->conn);
            return false;
        }
        rc = mpd_search_add_window(mpd_worker_state->mpd_state->conn, start, end);
        if (check_rc_error_and_recover(mpd_worker_state->mpd_state, NULL, NULL, 0, false, rc, "mpd_search_add_window") == false) {
            MYMPD_LOG_ERROR("Cache update failed");
            mpd_search_cancel(mpd_worker_state->mpd_state->conn);
            return false;
        }
        rc = mpd_search_commit(mpd_worker_state->mpd_state->conn);
        if (check_rc_error_and_recover(mpd_worker_state->mpd_state, NULL, NULL, 0, false, rc, "mpd_search_commit") == false) {
            MYMPD_LOG_ERROR("Cache update failed");
            return false;
        }
        struct mpd_song *song;
        sds album = sdsempty();
        sds artist = sdsempty();
        sds key = sdsempty();
        while ((song = mpd_recv_song(mpd_worker_state->mpd_state->conn)) != NULL) {
            //sticker cache
            if (feat_sticker == true) {
                const char *uri = mpd_song_get_uri(song);
                t_sticker *sticker = (t_sticker *) malloc(sizeof(t_sticker));
                assert(sticker);
                raxInsert(sticker_cache, (unsigned char*)uri, strlen(uri), (void *)sticker, NULL);
                song_count++;
            }

            //album cache
            if (feat_tags == true) {
                album = mpd_shared_get_tags(song, MPD_TAG_ALBUM, album);
                artist = mpd_shared_get_tags(song, MPD_TAG_ALBUM_ARTIST, artist);
                if (strcmp(album, "-") > 0 && strcmp(artist, "-") > 0) {
                    sdsclear(key);
                    key = sdscatfmt(key, "%s::%s", album, artist);
                    if (raxTryInsert(album_cache, (unsigned char*)key, sdslen(key), (void *)song, NULL) == 0) {
                        //discard song data if key exists
                        mpd_song_free(song);
                    }
                    else {
                        album_count++;
                    }
                }
                else {
                    MYMPD_LOG_WARN("Albumcache, skipping \"%s\"", mpd_song_get_uri(song));
                    mpd_song_free(song);
                }
            }
            i++;
        }
        sdsfree(album);
        sdsfree(artist);
        sdsfree(key);
        mpd_response_finish(mpd_worker_state->mpd_state->conn);
        if (check_error_and_recover2(mpd_worker_state->mpd_state, NULL, NULL, 0, false) == false) {
            MYMPD_LOG_ERROR("Cache update failed");
            return false;        
        }
        start = end;
        end = end + 1000;
    } while (i >= start);
    //get sticker values
    if (feat_sticker == true) {
        raxIterator iter;
        raxStart(&iter, sticker_cache);
        raxSeek(&iter, "^", NULL, 0);
        sds uri = sdsempty();
        while (raxNext(&iter)) {
            uri = sdsreplacelen(uri, (char *)iter.key, iter.key_len);
            mpd_shared_get_sticker(mpd_worker_state->mpd_state, uri, (t_sticker *)iter.data);
        }
        sdsfree(uri);
        raxStop(&iter);
    }
    MYMPD_LOG_INFO("Added %u albums to album cache", album_count);
    MYMPD_LOG_INFO("Added %u songs to sticker cache", song_count);
    MYMPD_LOG_INFO("Cache updated successfully");
    return true;
}
