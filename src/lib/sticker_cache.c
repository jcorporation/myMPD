/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2022 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include "mympd_config_defs.h"
#include "sticker_cache.h"

#include "../mpd_client/mpd_client_errorhandler.h"
#include "log.h"
#include "mem.h"
#include "sds_extras.h"
#include "utility.h"

#include <string.h>

//privat definitions
static bool _sticker_inc(struct t_cache *sticker_cache, struct t_mpd_state *mpd_state, 
        const char *uri, const char *name, const long value);
static bool _sticker_set(struct t_cache *sticker_cache, struct t_mpd_state *mpd_state,
        const char *uri, const char *name, const long long value);

//public functions

/** Gets the sticker struct from sticker cache
 * @param sticker_cache pointer to sticker cache
 * @param uri song uri
 * @return pointer to the sticker struct
 */
struct t_sticker *get_sticker_from_cache(struct t_cache *sticker_cache, const char *uri) {
    //ignore stream uris
    if (is_streamuri(uri) == true) {
        return NULL;
    }
    //check for uninitialized sticker cache
    if (sticker_cache->cache == NULL) {
        return NULL;
    }
    //try to get sticker
    void *data = raxFind(sticker_cache->cache, (unsigned char*)uri, strlen(uri));
    if (data == raxNotFound) {
        MYMPD_LOG_ERROR("Sticker for uri \"%s\" not found in cache", uri);
        return NULL;
    }
    return (struct t_sticker *) data;
}

/**
 * Frees the sticker cache
 * @param sticker_cache pointer to t_cache struct
 */
void sticker_cache_free(struct t_cache *sticker_cache) {
    if (sticker_cache->cache == NULL) {
        MYMPD_LOG_DEBUG("Sticker cache is NULL not freeing anything");
        return;
    }
    MYMPD_LOG_DEBUG("Freeing sticker cache");
    raxIterator iter;
    raxStart(&iter, sticker_cache->cache);
    raxSeek(&iter, "^", NULL, 0);
    while (raxNext(&iter)) {
        FREE_PTR(iter.data);
    }
    raxStop(&iter);
    raxFree(sticker_cache->cache);
    sticker_cache->cache = NULL;
}

/**
 * Increments the play count sticker by one
 * @param sticker_queue pointer to sticker queue
 * @param uri song uri
 * @return true on success else false
 */
bool sticker_inc_play_count(struct t_list *sticker_queue, const char *uri) {
    if (is_streamuri(uri) == true) {
        return true;
    }
    return list_push(sticker_queue, uri, 1, "playCount", NULL);
}

/**
 * Increments the skip count sticker by one
 * @param sticker_queue pointer to sticker queue
 * @param uri song uri
 * @return true on success else false
 */
bool sticker_inc_skip_count(struct t_list *sticker_queue, const char *uri) {
    if (is_streamuri(uri) == true) {
        return true;
    }
    return list_push(sticker_queue, uri, 1, "skipCount", NULL);
}

/**
 * Sets the like sticker value
 * @param sticker_queue pointer to sticker queue
 * @param uri song uri
 * @param value 0 = hate, 1 = neutral, 2 = like
 * @return true on success else false
 */
bool sticker_set_like(struct t_list *sticker_queue, const char *uri, int value) {
    if (is_streamuri(uri) == true) {
        return true;
    }
    return list_push(sticker_queue, uri, value, "like", NULL);
}

/**
 * Sets the last played time sticker
 * @param sticker_queue pointer to sticker queue
 * @param uri song uri
 * @param value start time of song
 * @return true on success else false
 */
bool sticker_set_last_played(struct t_list *sticker_queue, const char *uri, time_t song_start_time) {
    if (is_streamuri(uri) == true) {
        return true;
    }
    return list_push(sticker_queue, uri, (long long)song_start_time, "lastPlayed", NULL);
}

/**
 * Sets the last skipped time sticker
 * @param sticker_queue pointer to sticker queue
 * @param uri song uri
 * @return true on success else false
 */
bool sticker_set_last_skipped(struct t_list *sticker_queue, const char *uri) {
    if (is_streamuri(uri) == true) {
        return true;
    }
    time_t now = time(NULL);
    return list_push(sticker_queue, uri, (long long)now, "lastSkipped", NULL);
}

/**
 * Shifts through the sticker queue
 * @param sticker_queue pointer to sticker queue struct
 * @param sticker_cache pointer to sticker cache struct
 * @param mpd_state pointer to mpd_state struct
 * @return true on success else false
 */
bool sticker_dequeue(struct t_list *sticker_queue, struct t_cache *sticker_cache, struct t_mpd_state *mpd_state) {
    if (sticker_cache->cache == NULL ||
        sticker_cache->building == true)
    {
        //sticker cache is currently (re-)building in the mpd_worker thread
        //cache sticker write calls
        MYMPD_LOG_INFO("Delay setting stickers, sticker_cache is building");
        return false;
    }

    struct t_list_node *current;
    while ((current = list_shift_first(sticker_queue)) != NULL) {
        MYMPD_LOG_DEBUG("Setting %s = %lld for \"%s\"", current->value_p, current->value_i, current->key);
        if (strcmp(current->value_p, "playCount") == 0 ||
            strcmp(current->value_p, "skipCount") == 0)
        {
            _sticker_inc(sticker_cache, mpd_state, current->key, current->value_p, (long)current->value_i);
        }
        else if (strcmp(current->value_p, "like") == 0 ||
                 strcmp(current->value_p, "lastPlayed") == 0 ||
                 strcmp(current->value_p, "lastSkipped") == 0)
        {
            _sticker_set(sticker_cache, mpd_state, current->key, current->value_p, current->value_i);
        }
        list_node_free(current);
    }
    return true;
}

//private functions

/**
 * Increments a sticker by one in the cache and the mpd sticker database
 * @param sticker_cache pointer to sticker cache struct
 * @param mpd_state pointer to mpd_state struct
 * @param uri song uri
 * @param name sticker name
 * @param value value to increment by
 * @return true on success else false
 */
static bool _sticker_inc(struct t_cache *sticker_cache, struct t_mpd_state *mpd_state,
        const char *uri, const char *name, const long value)
{
    struct t_sticker *sticker = get_sticker_from_cache(sticker_cache, uri);
    if (sticker == NULL) {
        return false;
    }
    //update sticker cache
    long new_value = 0;
    if (strcmp(name, "playCount") == 0) {
        if (sticker->playCount + value > STICKER_PLAY_COUNT_MAX) {
            sticker->playCount = STICKER_PLAY_COUNT_MAX;
        }
        else {
            sticker->playCount += value;
        }
        new_value = sticker->playCount;
    }
    else if (strcmp(name, "skipCount") == 0) {
        if (sticker->skipCount + value > STICKER_SKIP_COUNT_MAX) {
            sticker->skipCount = STICKER_SKIP_COUNT_MAX;
        }
        else {
            sticker->skipCount += value;
        }
        new_value = sticker->skipCount;
    }
    else {
        MYMPD_LOG_ERROR("Invalid sticker name \"%s\"", name);
        return false;
    }

    //update mpd sticker
    sds value_str = sdsfromlonglong((long long)new_value);
    MYMPD_LOG_INFO("Setting sticker: \"%s\" -> %s: %s", uri, name, value_str);
    bool rc = mpd_run_sticker_set(mpd_state->conn, "song", uri, name, value_str);
    FREE_SDS(value_str);
    return mympd_check_rc_error_and_recover(mpd_state, rc, "mpd_run_sticker_set");
}

/**
 * Sets a sticker in the cache and the mpd sticker database
 * @param sticker_cache pointer to sticker cache struct
 * @param mpd_state pointer to mpd_state struct
 * @param uri song uri
 * @param name sticker name
 * @param value value to set
 * @return true on success else false
 */
static bool _sticker_set(struct t_cache *sticker_cache, struct t_mpd_state *mpd_state,
        const char *uri, const char *name, const long long value)
{
    sds value_str = sdsfromlonglong(value);
    MYMPD_LOG_INFO("Setting sticker: \"%s\" -> %s: %s", uri, name, value_str);
    //update sticker cache
    struct t_sticker *sticker = get_sticker_from_cache(sticker_cache, uri);
    if (sticker == NULL) {
        return false;
    }
    if (strcmp(name, "like") == 0) {
        sticker->like = (long)value;
    }
    else if (strcmp(name, "lastPlayed") == 0) {
        sticker->lastPlayed = (time_t)value;
    }
    else if (strcmp(name, "lastSkipped") == 0) {
        sticker->lastSkipped = (time_t)value;
    }
    else {
        MYMPD_LOG_ERROR("Invalid sticker name \"%s\"", name);
        return false;
    }

    //update mpd sticker
    bool rc = mpd_run_sticker_set(mpd_state->conn, "song", uri, name, value_str);
    FREE_SDS(value_str);
    return mympd_check_rc_error_and_recover(mpd_state, rc, "mpd_run_sticker_set");
}
