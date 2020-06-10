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
#include "mpd_client_utility.h"
#include "mpd_client_sticker.h"

//privat definitions
static bool _mpd_client_count_song_uri(t_mpd_client_state *mpd_client_state, const char *uri, const char *name, const int value);
static bool _mpd_client_set_sticker(t_mpd_client_state *mpd_client_state, const char *uri, const char *name, const int value);

//public functions
bool mpd_client_sticker_inc_play_count(t_mpd_client_state *mpd_client_state, const char *uri) {
    return list_push(&mpd_client_state->sticker_queue, uri, 1, "playCount", NULL);
}

bool mpd_client_sticker_inc_skip_count(t_mpd_client_state *mpd_client_state, const char *uri) {
    return list_push(&mpd_client_state->sticker_queue, uri, 1, "skipCount", NULL);
}

bool mpd_client_sticker_like(t_mpd_client_state *mpd_client_state, const char *uri, int value) {
    return list_push(&mpd_client_state->sticker_queue, uri, value, "like", NULL);
}

bool mpd_client_sticker_last_played(t_mpd_client_state *mpd_client_state, const char *uri) {
    return list_push(&mpd_client_state->sticker_queue, uri, mpd_client_state->song_start_time, "lastPlayed", NULL);
}

bool mpd_client_sticker_last_skipped(t_mpd_client_state *mpd_client_state, const char *uri) {
    time_t now = time(NULL);
    return list_push(&mpd_client_state->sticker_queue, uri, now, "lastSkipped", NULL);
}

bool mpd_client_sticker_dequeue(t_mpd_client_state *mpd_client_state) {
    if (mpd_client_state->sticker_cache != NULL && mpd_client_state->sticker_cache_building == true) {
        //sticker cache is currently (re-)building in the mpd_worker thread
        //cache sticker write calls
        return false;
    }
    
    struct list_node *current = mpd_client_state->sticker_queue.head;
    while (current != NULL) {
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

bool sticker_cache_init(t_config *config, t_mpd_client_state *mpd_client_state) {
    if (config->sticker_cache == false || mpd_client_state->feat_sticker == false || mpd_client_state->mpd_state->feat_mpd_searchwindow == false) {
        LOG_VERBOSE("Sticker cache is disabled, mpd version < 0.20.0 or stickers / sticker_cache not enabled");
        return false;
    }
    //push sticker cache building request to mpd_worker thread
    t_work_request *request = create_request(-1, 0, MPDWORKER_API_STICKERCACHE_CREATE, "MPDWORKER_API_STICKERCACHE_CREATE", "");
    request->data = sdscat(request->data, "{\"jsonrpc\":\"2.0\",\"id\":0,\"method\":\"MPDWORKER_API_STICKERCACHE_CREATE\",\"params\":{}}");
    tiny_queue_push(mpd_worker_queue, request);
    mpd_client_state->sticker_cache_building = true;
    return true;
}

struct t_sticker *get_sticker_from_cache(t_mpd_client_state *mpd_client_state, const char *uri) {
    void *data = raxFind(mpd_client_state->sticker_cache, (unsigned char*)uri, strlen(uri));
    if (data == raxNotFound) {
        return NULL;
    }
    t_sticker *sticker = (t_sticker *) data;
    return sticker;
}

bool mpd_client_get_sticker(t_mpd_client_state *mpd_client_state, const char *uri, t_sticker *sticker) {
    struct mpd_pair *pair;
    char *crap = NULL;
    sticker->playCount = 0;
    sticker->skipCount = 0;
    sticker->lastPlayed = 0;
    sticker->lastSkipped = 0;
    sticker->like = 1;

    if (uri == NULL || strstr(uri, "://") != NULL) {
        return false;
    }

    bool rc = mpd_send_sticker_list(mpd_client_state->mpd_state->conn, "song", uri);
    if (check_rc_error_and_recover(mpd_client_state->mpd_state, NULL, NULL, 0, false, rc, "mpd_send_sticker_list") == false) {
        return false;
    }

    while ((pair = mpd_recv_sticker(mpd_client_state->mpd_state->conn)) != NULL) {
        if (strcmp(pair->name, "playCount") == 0) {
            sticker->playCount = strtoimax(pair->value, &crap, 10);
        }
        else if (strcmp(pair->name, "skipCount") == 0) {
            sticker->skipCount = strtoimax(pair->value, &crap, 10);
        }
        else if (strcmp(pair->name, "lastPlayed") == 0) {
            sticker->lastPlayed = strtoimax(pair->value, &crap, 10);
        }
        else if (strcmp(pair->name, "lastSkipped") == 0) {
            sticker->lastSkipped = strtoimax(pair->value, &crap, 10);
        }
        else if (strcmp(pair->name, "like") == 0) {
            sticker->like = strtoimax(pair->value, &crap, 10);
        }
        mpd_return_sticker(mpd_client_state->mpd_state->conn, pair);
    }
    mpd_response_finish(mpd_client_state->mpd_state->conn);
    if (check_error_and_recover2(mpd_client_state->mpd_state, NULL, NULL, 0, false) == false) {
        return false;
    }

    return true;
}

//private functions
static bool _mpd_client_count_song_uri(t_mpd_client_state *mpd_client_state, const char *uri, const char *name, const int value) {
    if (uri == NULL || strstr(uri, "://") != NULL) {
        return false;
    }
    unsigned old_value = 0;
    t_sticker *sticker = NULL;
    if (mpd_client_state->sticker_cache != NULL) {
        sticker = get_sticker_from_cache(mpd_client_state, uri);
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
    LOG_VERBOSE("Setting sticker: \"%s\" -> %s: %s", uri, name, value_str);
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

static bool _mpd_client_set_sticker(t_mpd_client_state *mpd_client_state, const char *uri, const char *name, const int value) {
    if (uri == NULL || strstr(uri, "://") != NULL) {
        LOG_ERROR("Failed to set sticker %s to %d, invalid song uri: %s", name, value, uri);
        return false;
    }
    sds value_str = sdsfromlonglong(value);
    LOG_VERBOSE("Setting sticker: \"%s\" -> %s: %s", uri, name, value_str);
    bool rc = mpd_run_sticker_set(mpd_client_state->mpd_state->conn, "song", uri, name, value_str);
    sdsfree(value_str);
    if (check_rc_error_and_recover(mpd_client_state->mpd_state, NULL, NULL, 0, false, rc, "mpd_run_sticker_set") == false) {
        return false;
    }
    if (mpd_client_state->sticker_cache != NULL) {
        t_sticker *sticker = get_sticker_from_cache(mpd_client_state, uri);
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
