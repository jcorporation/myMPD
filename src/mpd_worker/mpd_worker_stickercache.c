/*
 SPDX-License-Identifier: GPL-2.0-or-later
 myMPD (c) 2018-2020 Juergen Mang <mail@jcgames.de>
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
#include "mpd_worker_stickercache.h"

//privat definitions
static bool _sticker_cache_init(t_mpd_worker_state *mpd_worker_state, rax *sticker_cache);

//public functions
bool mpd_worker_sticker_cache_init(t_mpd_worker_state *mpd_worker_state) {
    disable_all_mpd_tags(mpd_worker_state->mpd_state);
    rax *sticker_cache = raxNew();
    bool rc = _sticker_cache_init(mpd_worker_state, sticker_cache);
    enable_mpd_tags(mpd_worker_state->mpd_state, mpd_worker_state->mpd_state->mympd_tag_types);
    //push sticker cache building response to mpd_client thread
    t_work_request *request = create_request(-1, 0, MPD_API_STICKERCACHE_CREATED, "MPD_API_STICKERCACHE_CREATED", "");
    request->data = sdscat(request->data, "{\"jsonrpc\":\"2.0\",\"id\":0,\"method\":\"MPD_API_STICKERCACHE_CREATED\",\"params\":{}}");
    if (rc == true) {
        request->extra = (void *) sticker_cache;
    }
    else {
        sticker_cache_free(&sticker_cache);
    }
    tiny_queue_push(mpd_client_queue, request);
    return rc;
}

//private functions
static bool _sticker_cache_init(t_mpd_worker_state *mpd_worker_state, rax *sticker_cache) {
    LOG_VERBOSE("Creating sticker cache");
    unsigned start = 0;
    unsigned end = start + 1000;
    unsigned i = 0;
    struct mpd_song *song;
    //get all songs from database
    do {
        bool rc = mpd_search_db_songs(mpd_worker_state->mpd_state->conn, false);
        if (check_rc_error_and_recover(mpd_worker_state->mpd_state, NULL, NULL, 0, false, rc, "mpd_search_db_songs") == false) {
            LOG_ERROR("Sticker cache update failed");
            mpd_search_cancel(mpd_worker_state->mpd_state->conn);
            return false;
        }
        rc = mpd_search_add_uri_constraint(mpd_worker_state->mpd_state->conn, MPD_OPERATOR_DEFAULT, "");
        if (check_rc_error_and_recover(mpd_worker_state->mpd_state, NULL, NULL, 0, false, rc, "mpd_search_add_uri_constraint") == false) {
            LOG_ERROR("Sticker cache update failed");
            mpd_search_cancel(mpd_worker_state->mpd_state->conn);
            return false;
        }
        rc = mpd_search_add_window(mpd_worker_state->mpd_state->conn, start, end);
        if (check_rc_error_and_recover(mpd_worker_state->mpd_state, NULL, NULL, 0, false, rc, "mpd_search_add_window") == false) {
            LOG_ERROR("Sticker cache update failed");
            mpd_search_cancel(mpd_worker_state->mpd_state->conn);
            return false;
        }
        rc = mpd_search_commit(mpd_worker_state->mpd_state->conn);
        if (check_rc_error_and_recover(mpd_worker_state->mpd_state, NULL, NULL, 0, false, rc, "mpd_search_commit") == false) {
            LOG_ERROR("Sticker cache update failed");
            return false;
        }
        
        while ((song = mpd_recv_song(mpd_worker_state->mpd_state->conn)) != NULL) {
            const char *uri = mpd_song_get_uri(song);
            t_sticker *sticker = (t_sticker *) malloc(sizeof(t_sticker));
            assert(sticker);
            raxInsert(sticker_cache, (unsigned char*)uri, strlen(uri), (void *)sticker, NULL);
            i++;
            mpd_song_free(song);
        }
        mpd_response_finish(mpd_worker_state->mpd_state->conn);
        if (check_error_and_recover2(mpd_worker_state->mpd_state, NULL, NULL, 0, false) == false) {
            sticker_cache_free(&sticker_cache);
            LOG_ERROR("Sticker cache update failed");
            return false;        
        }
        start = end;
        end = end + 1000;
    } while (i >= start);
    //get sticker values
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
    LOG_VERBOSE("Sticker cache updated successfully");
    return true;
}
