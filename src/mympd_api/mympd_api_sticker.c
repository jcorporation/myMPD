/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2022 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include "mympd_config_defs.h"
#include "mympd_api_sticker.h"

#include "../lib/api.h"
#include "../lib/log.h"
#include "../lib/sds_extras.h"
#include "../lib/utility.h"
#include "../lib/validate.h"
#include "../mpd_shared.h"
#include "../mpd_shared/mpd_shared_sticker.h"


#include <inttypes.h>
#include <limits.h>
#include <string.h>

//privat definitions
static bool _mympd_api_count_song_uri(struct t_mympd_state *mympd_state, const char *uri, const char *name, const long value);
static bool _mympd_api_set_sticker(struct t_mympd_state *mympd_state, const char *uri, const char *name, const long value);

//public functions
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
    return list_push(&mympd_state->sticker_queue, uri, mympd_state->mpd_state->song_start_time, "lastPlayed", NULL);
}

bool mympd_api_sticker_last_skipped(struct t_mympd_state *mympd_state, const char *uri) {
    if (is_streamuri(uri) == true) {
        return true;
    }
    time_t now = time(NULL);
    return list_push(&mympd_state->sticker_queue, uri, now, "lastSkipped", NULL);
}

bool mympd_api_sticker_dequeue(struct t_mympd_state *mympd_state) {
    if (mympd_state->sticker_cache != NULL && mympd_state->sticker_cache_building == true) {
        //sticker cache is currently (re-)building in the mpd_worker thread
        //cache sticker write calls
        MYMPD_LOG_INFO("Delay setting stickers, sticker_cache is building");
        return false;
    }

    struct t_list_node *current = mympd_state->sticker_queue.head;
    while (current != NULL) {
        MYMPD_LOG_DEBUG("Setting %s = %ld for %s", current->value_p, current->value_i, current->key);
        if (strcmp(current->value_p, "playCount") == 0 || strcmp(current->value_p, "skipCount") == 0) {
            _mympd_api_count_song_uri(mympd_state, current->key, current->value_p, current->value_i);
        }
        else if (strcmp(current->value_p, "like") == 0 || strcmp(current->value_p, "lastPlayed") == 0 ||
                 strcmp(current->value_p, "lastSkipped") == 0)
        {
            _mympd_api_set_sticker(mympd_state, current->key, current->value_p, current->value_i);
        }
        list_shift(&mympd_state->sticker_queue, 0);
        current = mympd_state->sticker_queue.head;
    }
    return true;
}

//private functions
static bool _mympd_api_count_song_uri(struct t_mympd_state *mympd_state, const char *uri, const char *name, const long value) {
    long old_value = 0;
    struct t_sticker *sticker = NULL;
    if (mympd_state->sticker_cache != NULL) {
        sticker = get_sticker_from_cache(mympd_state->sticker_cache, uri);
        if (sticker != NULL) {
            if (strcmp(name, "playCount") == 0) {
                old_value = sticker->playCount;
            }
            else if (strcmp(name, "skipCount") == 0) {
                old_value = sticker->skipCount;
            }
        }
    }
    else {
        struct mpd_pair *pair;
        char *crap = NULL;
        bool rc = mpd_send_sticker_list(mympd_state->mpd_state->conn, "song", uri);
        if (check_rc_error_and_recover(mympd_state->mpd_state, NULL, NULL, 0, false, rc, "mpd_send_sticker_list") == false) {
            return false;
        }
        while ((pair = mpd_recv_sticker(mympd_state->mpd_state->conn)) != NULL) {
            if (strcmp(pair->name, name) == 0) {
                old_value = (long)strtoimax(pair->value, &crap, 10);
            }
            mpd_return_sticker(mympd_state->mpd_state->conn, pair);
        }
        mpd_response_finish(mympd_state->mpd_state->conn);
        if (check_error_and_recover2(mympd_state->mpd_state, NULL, NULL, 0, true) == false) {
            return false;
        }
    }

    old_value += value;
    if (old_value > INT_MAX / 2) {
        old_value = INT_MAX / 2;
    }

    sds value_str = sdsfromlonglong((long long)old_value);
    MYMPD_LOG_INFO("Setting sticker: \"%s\" -> %s: %s", uri, name, value_str);
    bool rc = mpd_run_sticker_set(mympd_state->mpd_state->conn, "song", uri, name, value_str);
    FREE_SDS(value_str);
    if (rc == false) {
        check_error_and_recover(mympd_state->mpd_state, NULL, NULL, 0);
    }
    else {
        if (sticker != NULL) {
            if (strcmp(name, "playCount") == 0) {
                sticker->playCount = old_value;
            }
            else if (strcmp(name, "skipCount") == 0) {
                sticker->skipCount = old_value;
            }
        }
    }
    return rc;
}

static bool _mympd_api_set_sticker(struct t_mympd_state *mympd_state, const char *uri, const char *name, const long value) {
    sds value_str = sdsfromlonglong((long long)value);
    MYMPD_LOG_INFO("Setting sticker: \"%s\" -> %s: %s", uri, name, value_str);
    bool rc = mpd_run_sticker_set(mympd_state->mpd_state->conn, "song", uri, name, value_str);
    FREE_SDS(value_str);
    if (check_rc_error_and_recover(mympd_state->mpd_state, NULL, NULL, 0, false, rc, "mpd_run_sticker_set") == false) {
        return false;
    }
    if (mympd_state->sticker_cache != NULL) {
        struct t_sticker *sticker = get_sticker_from_cache(mympd_state->sticker_cache, uri);
        if (sticker != NULL) {
            if (strcmp(name, "like") == 0) {
                sticker->like = value;
            }
            else if (strcmp(name, "lastPlayed") == 0) {
                sticker->lastPlayed = value;
            }
            else if (strcmp(name, "lastSkipped") == 0) {
                sticker->lastSkipped = value;
            }
        }
    }
    return true;
}
