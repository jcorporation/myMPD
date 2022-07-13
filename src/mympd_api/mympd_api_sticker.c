/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2022 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include "mympd_config_defs.h"
#include "mympd_api_sticker.h"

#include "../lib/api.h"
#include "../lib/jsonrpc.h"
#include "../lib/log.h"
#include "../lib/sticker_cache.h"
#include "../lib/sds_extras.h"
#include "../lib/utility.h"
#include "../lib/validate.h"
#include "../mpd_client/mpd_client_errorhandler.h"

#include <inttypes.h>
#include <limits.h>
#include <string.h>

//privat definitions
static bool _mympd_api_sticker_count(struct t_mympd_state *mympd_state, const char *uri, const char *name, const long value);
static bool _mympd_api_sticker_set(struct t_mympd_state *mympd_state, const char *uri, const char *name, const long long value);

//public functions
/**
 * Gets the stickers from sticker cache and returns a json list
 * Shortcut for get_sticker_from_cache and print_sticker
 * @param buffer already allocated sds string to append the list
 * @param sticker_cache pointer to sticker cache
 * @param uri song uri
 * @return pointer to the modified buffer
 */
sds mympd_api_sticker_list(sds buffer, rax *sticker_cache, const char *uri) {
    struct t_sticker *sticker = get_sticker_from_cache(sticker_cache, uri);
    return mympd_api_print_sticker(buffer, sticker);
}

/**
 * Print the sticker struct as json list
 * @param buffer already allocated sds string to append the list
 * @param sticker pointer to sticker struct to print
 * @return pointer to the modified buffer
 */
sds mympd_api_print_sticker(sds buffer, struct t_sticker *sticker) {
    if (sticker != NULL) {
        buffer = tojson_long(buffer, "stickerPlayCount", sticker->playCount, true);
        buffer = tojson_long(buffer, "stickerSkipCount", sticker->skipCount, true);
        buffer = tojson_long(buffer, "stickerLike", sticker->like, true);
        buffer = tojson_llong(buffer, "stickerLastPlayed", (long long)sticker->lastPlayed, true);
        buffer = tojson_llong(buffer, "stickerLastSkipped", (long long)sticker->lastSkipped, false);
    }
    else {
        buffer = tojson_long(buffer, "stickerPlayCount", 0, true);
        buffer = tojson_long(buffer, "stickerSkipCount", 0, true);
        buffer = tojson_long(buffer, "stickerLike", 1, true);
        buffer = tojson_long(buffer, "stickerLastPlayed", 0, true);
        buffer = tojson_long(buffer, "stickerLastSkipped", 0, false);
    }
    return buffer;
}

bool mympd_api_sticker_inc_play_count(struct t_mympd_state *mympd_state, const char *uri) {
    if (is_streamuri(uri) == true) {
        return true;
    }
    return list_push(&mympd_state->sticker_queue, uri, 1, "playCount", NULL);
}

bool mympd_api_sticker_inc_skip_count(struct t_mympd_state *mympd_state, const char *uri) {
    if (is_streamuri(uri) == true) {
        return true;
    }
    return list_push(&mympd_state->sticker_queue, uri, 1, "skipCount", NULL);
}

bool mympd_api_sticker_like(struct t_mympd_state *mympd_state, const char *uri, int value) {
    if (is_streamuri(uri) == true) {
        return true;
    }
    return list_push(&mympd_state->sticker_queue, uri, value, "like", NULL);
}

bool mympd_api_sticker_last_played(struct t_mympd_state *mympd_state, const char *uri) {
    if (is_streamuri(uri) == true) {
        return true;
    }
    return list_push(&mympd_state->sticker_queue, uri, (long long)mympd_state->mpd_state->song_start_time, "lastPlayed", NULL);
}

bool mympd_api_sticker_last_skipped(struct t_mympd_state *mympd_state, const char *uri) {
    if (is_streamuri(uri) == true) {
        return true;
    }
    time_t now = time(NULL);
    return list_push(&mympd_state->sticker_queue, uri, (long long)now, "lastSkipped", NULL);
}

bool mympd_api_sticker_dequeue(struct t_mympd_state *mympd_state) {
    if (mympd_state->sticker_cache == NULL ||
        mympd_state->sticker_cache_building == true)
    {
        //sticker cache is currently (re-)building in the mpd_worker thread
        //cache sticker write calls
        MYMPD_LOG_INFO("Delay setting stickers, sticker_cache is building");
        return false;
    }

    struct t_list_node *current;
    while ((current = list_shift_first(&mympd_state->sticker_queue)) != NULL) {
        MYMPD_LOG_DEBUG("Setting %s = %lld for \"%s\"", current->value_p, current->value_i, current->key);
        if (strcmp(current->value_p, "playCount") == 0 ||
            strcmp(current->value_p, "skipCount") == 0)
        {
            _mympd_api_sticker_count(mympd_state, current->key, current->value_p, (long)current->value_i);
        }
        else if (strcmp(current->value_p, "like") == 0 ||
                 strcmp(current->value_p, "lastPlayed") == 0 ||
                 strcmp(current->value_p, "lastSkipped") == 0)
        {
            _mympd_api_sticker_set(mympd_state, current->key, current->value_p, current->value_i);
        }
        list_node_free(current);
    }
    return true;
}

//private functions
static bool _mympd_api_sticker_count(struct t_mympd_state *mympd_state, const char *uri, const char *name, const long value) {
    struct t_sticker *sticker = get_sticker_from_cache(mympd_state->sticker_cache, uri);
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
    bool rc = mpd_run_sticker_set(mympd_state->mpd_state->conn, "song", uri, name, value_str);
    FREE_SDS(value_str);
    if (check_rc_error_and_recover(mympd_state->mpd_state, NULL, NULL, 0, false, rc, "mpd_run_sticker_set") == false) {
        return false;
    }
    return true;
}

static bool _mympd_api_sticker_set(struct t_mympd_state *mympd_state, const char *uri, const char *name, const long long value) {
    sds value_str = sdsfromlonglong(value);
    MYMPD_LOG_INFO("Setting sticker: \"%s\" -> %s: %s", uri, name, value_str);
    //update sticker cache
    struct t_sticker *sticker = get_sticker_from_cache(mympd_state->sticker_cache, uri);
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
    bool rc = mpd_run_sticker_set(mympd_state->mpd_state->conn, "song", uri, name, value_str);
    FREE_SDS(value_str);
    if (check_rc_error_and_recover(mympd_state->mpd_state, NULL, NULL, 0, false, rc, "mpd_run_sticker_set") == false) {
        return false;
    }

    return true;
}
