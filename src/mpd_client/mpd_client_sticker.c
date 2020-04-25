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
#include <mpd/client.h>

#include "../../dist/src/sds/sds.h"
#include "../sds_extras.h"
#include "../api.h"
#include "../log.h"
#include "../list.h"
#include "config_defs.h"
#include "../utility.h"
#include "mpd_client_utility.h"
#include "mpd_client_sticker.h"

//privat definitions
bool _sticker_cache_init(t_config *config, t_mpd_state *mpd_state);

//public functions
bool sticker_cache_init(t_config *config, t_mpd_state *mpd_state) {
    if (mpd_connection_cmp_server_version(mpd_state->conn, 0, 20, 0) >= 0) {
        disable_all_mpd_tags(mpd_state);
        bool rc = _sticker_cache_init(config, mpd_state);
        enable_mpd_tags(mpd_state, mpd_state->mympd_tag_types);
        return rc;
    }
    else {
        LOG_WARN("Sticker cache disabled, mpd version < 0.20.0");
        return false;
    }
}

bool mpd_client_count_song_uri(t_mpd_state *mpd_state, const char *uri, const char *name, const int value) {
    if (uri == NULL || strstr(uri, "://") != NULL) {
        return false;
    }
    int old_value = 0;
    t_sticker *sticker = NULL;
    if (mpd_state->sticker_cache != NULL) {
        sticker = get_sticker_from_cache(mpd_state, uri);
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
        bool rc = mpd_send_sticker_list(mpd_state->conn, "song", uri);
        if (check_rc_error_and_recover(mpd_state, NULL, NULL, 0, false, rc, "mpd_send_sticker_list") == false) {
            return false;
        }
        while ((pair = mpd_recv_sticker(mpd_state->conn)) != NULL) {
            if (strcmp(pair->name, name) == 0) {
                old_value = strtoimax(pair->value, &crap, 10);
            }
            mpd_return_sticker(mpd_state->conn, pair);
        }
        mpd_response_finish(mpd_state->conn);
        if (check_error_and_recover2(mpd_state, NULL, NULL, 0, true) == false) {
            return false;
        }
    }

    old_value += value;
    if (old_value > INT_MAX / 2) {
        old_value = INT_MAX / 2;
    }
    else if (old_value < 0) {
        old_value = 0;
    }
    sds value_str = sdsfromlonglong(old_value);
    LOG_VERBOSE("Setting sticker: \"%s\" -> %s: %s", uri, name, value_str);
    bool rc = mpd_run_sticker_set(mpd_state->conn, "song", uri, name, value_str);
    sdsfree(value_str);
    if (rc == false) {
        check_error_and_recover(mpd_state, NULL, NULL, 0);
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

sds mpd_client_like_song_uri(t_mpd_state *mpd_state, sds buffer, sds method, int request_id,
                             const char *uri, int value)
{
    if (uri == NULL || strstr(uri, "://") != NULL) {
        buffer = jsonrpc_respond_message(buffer, method, request_id, "Failed to set like, invalid song uri", true);
        return buffer;
    }
    if (value > 2 || value < 0) {
        buffer = jsonrpc_respond_message(buffer, method, request_id, "Failed to set like, invalid like value", true);
        return buffer;
    }
    sds value_str = sdsfromlonglong(value);
    LOG_VERBOSE("Setting sticker: \"%s\" -> like: %s", uri, value_str);
    bool rc = mpd_run_sticker_set(mpd_state->conn, "song", uri, "like", value_str);
    sdsfree(value_str);
    if (check_rc_error_and_recover(mpd_state, NULL, NULL, 0, false, rc, "mpd_run_sticker_set") == false) {
        return buffer;
    }
    if (mpd_state->sticker_cache != NULL) {
        t_sticker *sticker = get_sticker_from_cache(mpd_state, uri);
        if (sticker != NULL) {
            sticker->like = value;
        }
    }
    buffer = jsonrpc_respond_ok(buffer, method, request_id);
    return buffer;        
}

bool mpd_client_last_played_song_uri(t_mpd_state *mpd_state, const char *uri) {
    if (uri == NULL || strstr(uri, "://") != NULL) {
        return false;
    }
    sds value_str = sdsfromlonglong(mpd_state->song_start_time);
    LOG_VERBOSE("Setting sticker: \"%s\" -> lastPlayed: %s", uri, value_str);
    bool rc = mpd_run_sticker_set(mpd_state->conn, "song", uri, "lastPlayed", value_str);
    sdsfree(value_str);
    if (check_rc_error_and_recover(mpd_state, NULL, NULL, 0, false, rc, "mpd_run_sticker_set") == false) {
        return false;
    } 
    if (mpd_state->sticker_cache != NULL) {
        t_sticker *sticker = get_sticker_from_cache(mpd_state, uri);
        if (sticker != NULL) {
            sticker->lastPlayed = mpd_state->song_start_time;
        }
    }
    return true;
}

bool mpd_client_last_skipped_song_uri(t_mpd_state *mpd_state, const char *uri) {
    if (uri == NULL || strstr(uri, "://") != NULL) {
        return false;
    }
    time_t now = time(NULL);
    sds value_str = sdsfromlonglong(now);
    LOG_VERBOSE("Setting sticker: \"%s\" -> lastSkipped: %s", uri, value_str);
    bool rc = mpd_run_sticker_set(mpd_state->conn, "song", uri, "lastSkipped", value_str);
    sdsfree(value_str);
    if (check_rc_error_and_recover(mpd_state, NULL, NULL, 0, false, rc, "mpd_run_sticker_set") == false) {
        return false;
    }
    if (mpd_state->sticker_cache != NULL) {
        t_sticker *sticker = get_sticker_from_cache(mpd_state, uri);
        if (sticker != NULL) {
            sticker->lastSkipped = now;
        }
    }
    return true;
}

struct t_sticker *get_sticker_from_cache(t_mpd_state *mpd_state, const char *uri) {
    void *data = raxFind(mpd_state->sticker_cache, (unsigned char*)uri, strlen(uri));
    if (data == raxNotFound) {
        return NULL;
    }
    t_sticker *sticker = (t_sticker *) data;
    return sticker;
}

bool mpd_client_get_sticker(t_mpd_state *mpd_state, const char *uri, t_sticker *sticker) {
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

    bool rc = mpd_send_sticker_list(mpd_state->conn, "song", uri);
    if (check_rc_error_and_recover(mpd_state, NULL, NULL, 0, false, rc, "mpd_send_sticker_list") == false) {
        return false;
    }

    while ((pair = mpd_recv_sticker(mpd_state->conn)) != NULL) {
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
        mpd_return_sticker(mpd_state->conn, pair);
    }
    mpd_response_finish(mpd_state->conn);
    if (check_error_and_recover2(mpd_state, NULL, NULL, 0, false) == false) {
        return false;
    }

    return true;
}

void sticker_cache_free(t_mpd_state *mpd_state) {
    if (mpd_state->sticker_cache == NULL) {
        return;
    }
    raxIterator iter;
    raxStart(&iter, mpd_state->sticker_cache);
    raxSeek(&iter, "^", NULL, 0);
    while (raxNext(&iter)) {
        FREE_PTR(iter.data);
    }
    raxStop(&iter);
    raxFree(mpd_state->sticker_cache);
    mpd_state->sticker_cache = NULL;
}

//private functions
bool _sticker_cache_init(t_config *config, t_mpd_state *mpd_state) {
    if (config->sticker_cache == false || mpd_state->feat_sticker == false || mpd_state->feat_mpd_searchwindow == false) {
        return false;
    }
    LOG_VERBOSE("Updating sticker cache");
    unsigned start = 0;
    unsigned end = start + 1000;
    unsigned i = 0;
    struct mpd_song *song;
    mpd_state->sticker_cache = raxNew();
    //get all songs from database
    do {
        bool rc = mpd_search_db_songs(mpd_state->conn, false);
        if (check_rc_error_and_recover(mpd_state, NULL, NULL, 0, false, rc, "mpd_search_db_songs") == false) {
            LOG_ERROR("Sticker cache update failed");
            mpd_search_cancel(mpd_state->conn);
            return false;
        }
        rc = mpd_search_add_uri_constraint(mpd_state->conn, MPD_OPERATOR_DEFAULT, "");
        if (check_rc_error_and_recover(mpd_state, NULL, NULL, 0, false, rc, "mpd_search_add_uri_constraint") == false) {
            LOG_ERROR("Sticker cache update failed");
            mpd_search_cancel(mpd_state->conn);
            return false;
        }
        rc = mpd_search_add_window(mpd_state->conn, start, end);
        if (check_rc_error_and_recover(mpd_state, NULL, NULL, 0, false, rc, "mpd_search_add_window") == false) {
            LOG_ERROR("Sticker cache update failed");
            mpd_search_cancel(mpd_state->conn);
            return false;
        }
        rc = mpd_search_commit(mpd_state->conn);
        if (check_rc_error_and_recover(mpd_state, NULL, NULL, 0, false, rc, "mpd_search_commit") == false) {
            LOG_ERROR("Sticker cache update failed");
            return false;
        }
        
        while ((song = mpd_recv_song(mpd_state->conn)) != NULL) {
            const char *uri = mpd_song_get_uri(song);
            t_sticker *sticker = (t_sticker *) malloc(sizeof(t_sticker));
            assert(sticker);
            raxInsert(mpd_state->sticker_cache, (unsigned char*)uri, strlen(uri), (void *)sticker, NULL);
            i++;
            mpd_song_free(song);
        }
        mpd_response_finish(mpd_state->conn);
        if (check_error_and_recover2(mpd_state, NULL, NULL, 0, false) == false) {
            sticker_cache_free(mpd_state);
            LOG_ERROR("Sticker cache update failed");
            return false;        
        }
        start = end;
        end = end + 1000;
    } while (i >= start);
    //get sticker values
    raxIterator iter;
    raxStart(&iter, mpd_state->sticker_cache);
    raxSeek(&iter, "^", NULL, 0);
    sds uri = sdsempty();
    while (raxNext(&iter)) {
        uri = sdsreplacelen(uri, (char *)iter.key, iter.key_len);
        mpd_client_get_sticker(mpd_state, uri, (t_sticker *)iter.data);
    }
    sdsfree(uri);
    raxStop(&iter);
    LOG_VERBOSE("Sticker cache updated successfully");
    return true;
}
