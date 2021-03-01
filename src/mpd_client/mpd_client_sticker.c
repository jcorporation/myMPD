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
#include "../mpd_shared/mpd_shared_sticker.h"
#include "../mpd_shared.h"
#include "mpd_client_utility.h"
#include "mpd_client_sticker.h"

//privat definitions
static bool _mpd_client_count_song_uri(t_mpd_client_state *mpd_client_state, const char *uri, const char *name, const long value);
static bool _mpd_client_set_sticker(t_mpd_client_state *mpd_client_state, const char *uri, const char *name, const long value);

//public functions
bool mpd_client_sticker_inc_play_count(t_mpd_client_state *mpd_client_state, const char *uri) {
    if (is_streamuri(uri) == true) {
        return true;
    }
    return list_push(&mpd_client_state->sticker_queue, uri, 1, "playCount", NULL);
}

bool mpd_client_sticker_inc_skip_count(t_mpd_client_state *mpd_client_state, const char *uri) {
    if (is_streamuri(uri) == true) {
        return true;
    }
    return list_push(&mpd_client_state->sticker_queue, uri, 1, "skipCount", NULL);
}

bool mpd_client_sticker_like(t_mpd_client_state *mpd_client_state, const char *uri, int value) {
    if (is_streamuri(uri) == true) {
        return true;
    }
    return list_push(&mpd_client_state->sticker_queue, uri, value, "like", NULL);
}

bool mpd_client_sticker_last_played(t_mpd_client_state *mpd_client_state, const char *uri) {
    if (is_streamuri(uri) == true) {
        return true;
    }
    return list_push(&mpd_client_state->sticker_queue, uri, mpd_client_state->song_start_time, "lastPlayed", NULL);
}

bool mpd_client_sticker_last_skipped(t_mpd_client_state *mpd_client_state, const char *uri) {
    if (is_streamuri(uri) == true) {
        return true;
    }
    time_t now = time(NULL);
    return list_push(&mpd_client_state->sticker_queue, uri, now, "lastSkipped", NULL);
}

bool mpd_client_sticker_dequeue(t_mpd_client_state *mpd_client_state) {
    if (mpd_client_state->sticker_cache != NULL && mpd_client_state->sticker_cache_building == true) {
        //sticker cache is currently (re-)building in the mpd_worker thread
        //cache sticker write calls
        MYMPD_LOG_INFO("Delay setting stickers, sticker_cache is building");
        return false;
    }
    
    struct list_node *current = mpd_client_state->sticker_queue.head;
    while (current != NULL) {
        MYMPD_LOG_DEBUG("Setting %s = %ld for %s", current->value_p, current->value_i, current->key);
        if (strcmp(current->value_p, "playCount") == 0 || strcmp(current->value_p, "skipCount") == 0) {
            _mpd_client_count_song_uri(mpd_client_state, current->key, current->value_p, current->value_i);
        }
        else if (strcmp(current->value_p, "like") == 0 || strcmp(current->value_p, "lastPlayed") == 0 || 
                 strcmp(current->value_p, "lastSkipped") == 0) 
        {
            _mpd_client_set_sticker(mpd_client_state, current->key, current->value_p, current->value_i);
        }
        list_shift(&mpd_client_state->sticker_queue, 0);
        current = mpd_client_state->sticker_queue.head;
    }
    return true;
}

//private functions
static bool _mpd_client_count_song_uri(t_mpd_client_state *mpd_client_state, const char *uri, const char *name, const long value) {
    unsigned old_value = 0;
    t_sticker *sticker = NULL;
    if (mpd_client_state->sticker_cache != NULL) {
        sticker = get_sticker_from_cache(mpd_client_state->sticker_cache, uri);
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
        bool rc = mpd_send_sticker_list(mpd_client_state->mpd_state->conn, "song", uri);
        if (check_rc_error_and_recover(mpd_client_state->mpd_state, NULL, NULL, 0, false, rc, "mpd_send_sticker_list") == false) {
            return false;
        }
        while ((pair = mpd_recv_sticker(mpd_client_state->mpd_state->conn)) != NULL) {
            if (strcmp(pair->name, name) == 0) {
                old_value = strtoimax(pair->value, &crap, 10);
            }
            mpd_return_sticker(mpd_client_state->mpd_state->conn, pair);
        }
        mpd_response_finish(mpd_client_state->mpd_state->conn);
        if (check_error_and_recover2(mpd_client_state->mpd_state, NULL, NULL, 0, true) == false) {
            return false;
        }
    }

    old_value += value;
    if (old_value > INT_MAX / 2) {
        old_value = INT_MAX / 2;
    }

    sds value_str = sdsfromlonglong(old_value);
    MYMPD_LOG_INFO("Setting sticker: \"%s\" -> %s: %s", uri, name, value_str);
    bool rc = mpd_run_sticker_set(mpd_client_state->mpd_state->conn, "song", uri, name, value_str);
    sdsfree(value_str);
    if (rc == false) {
        check_error_and_recover(mpd_client_state->mpd_state, NULL, NULL, 0);
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

static bool _mpd_client_set_sticker(t_mpd_client_state *mpd_client_state, const char *uri, const char *name, const long value) {
    sds value_str = sdsfromlonglong(value);
    MYMPD_LOG_INFO("Setting sticker: \"%s\" -> %s: %s", uri, name, value_str);
    bool rc = mpd_run_sticker_set(mpd_client_state->mpd_state->conn, "song", uri, name, value_str);
    sdsfree(value_str);
    if (check_rc_error_and_recover(mpd_client_state->mpd_state, NULL, NULL, 0, false, rc, "mpd_run_sticker_set") == false) {
        return false;
    }
    if (mpd_client_state->sticker_cache != NULL) {
        t_sticker *sticker = get_sticker_from_cache(mpd_client_state->sticker_cache, uri);
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
