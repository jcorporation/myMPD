/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2024 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include "compile_time.h"
#include "src/mpd_client/stickerdb.h"

#include "dist/rax/rax.h"
#include "src/lib/convert.h"
#include "src/lib/jsonrpc.h"
#include "src/lib/log.h"
#include "src/lib/mympd_state.h"
#include "src/lib/sds_extras.h"
#include "src/lib/sticker.h"
#include "src/lib/utility.h"
#include "src/mympd_api/requests.h"

#include <inttypes.h>
#include <limits.h>
#include <poll.h>
#include <stdlib.h>
#include <string.h>

// Private definitions

static bool sticker_search_add_value_constraint(struct t_stickerdb_state *stickerdb, enum mpd_sticker_operator op, const char *value);
static bool sticker_search_add_sort(struct t_stickerdb_state *stickerdb, enum mpd_sticker_sort sort, bool desc);
static bool sticker_search_add_window(struct t_stickerdb_state *stickerdb, unsigned start, unsigned end);

static struct t_sticker *get_sticker_all(struct t_stickerdb_state *stickerdb, const char *uri, struct t_sticker *sticker, bool user_defined);
static sds get_sticker_value(struct t_stickerdb_state *stickerdb, const char *uri, const char *name);
static int64_t get_sticker_int64(struct t_stickerdb_state *stickerdb, const char *uri, const char *name);
static bool set_sticker_value(struct t_stickerdb_state *stickerdb, const char *uri, const char *name, const char *value);
static bool set_sticker_int64(struct t_stickerdb_state *stickerdb, const char *uri, const char *name, int64_t value);
static bool inc_sticker(struct t_stickerdb_state *stickerdb, const char *uri, const char *name);
static bool remove_sticker(struct t_stickerdb_state *stickerdb, const char *uri, const char *name);
static bool stickerdb_connect_mpd(struct t_stickerdb_state *stickerdb);
static bool check_sticker_support(struct t_stickerdb_state *stickerdb);

// Public functions

/**
 * This function connects to mpd sticker instance on demand
 * or exits the idle mode
 * @param stickerdb pointer to the stickerdb state
 */
bool stickerdb_connect(struct t_stickerdb_state *stickerdb) {
    if (stickerdb->config->stickers == false) {
        MYMPD_LOG_WARN("stickerdb", "Stickers are disabled by config");
        return false;
    }
    if (stickerdb->conn_state == MPD_FAILURE) {
        MYMPD_LOG_DEBUG("stickerdb", "Disconnecting from MPD");
        stickerdb_disconnect(stickerdb);
    }
    if (stickerdb->conn_state == MPD_CONNECTED) {
        // already connected
        MYMPD_LOG_DEBUG("stickerdb", "Connected, leaving idle mode");
        if (stickerdb_exit_idle(stickerdb) == true) {
            return true;
        }
        // stickerdb connection broken
        stickerdb_disconnect(stickerdb);
    }
    // try to connect
    MYMPD_LOG_INFO(stickerdb->name, "Creating mpd connection for %s", stickerdb->name);
    if (stickerdb_connect_mpd(stickerdb) == false) {
        MYMPD_LOG_DEBUG("stickerdb", "Connecting to MPD");
        return false;
    }
    // check version
    if (mpd_connection_cmp_server_version(stickerdb->conn, MPD_VERSION_MIN_MAJOR, MPD_VERSION_MIN_MINOR, MPD_VERSION_MIN_PATCH) < 0) {
        MYMPD_LOG_DEBUG("stickerdb", "Checking version");
        MYMPD_LOG_ERROR(stickerdb->name, "MPD version too old, myMPD supports only MPD version >= 0.21");
        stickerdb_disconnect(stickerdb);
        send_jsonrpc_notify(JSONRPC_FACILITY_MPD, JSONRPC_SEVERITY_ERROR, MPD_PARTITION_ALL, "MPD version is too old");
        mympd_api_request_sticker_features(false, false, false);
        return false;
    }
    // check for sticker support
    stickerdb->mpd_state->feat.stickers = check_sticker_support(stickerdb);
    stickerdb->mpd_state->feat.sticker_sort_window = false;
    stickerdb->mpd_state->feat.sticker_int = false;

    if (stickerdb->mpd_state->feat.stickers == false) {
        MYMPD_LOG_ERROR("stickerdb", "MPD does not support stickers");
        stickerdb_disconnect(stickerdb);
        send_jsonrpc_notify(JSONRPC_FACILITY_MPD, JSONRPC_SEVERITY_ERROR, MPD_PARTITION_ALL, "MPD does not support stickers");
        mympd_api_request_sticker_features(false, false, false);
        return false;
    }
    #ifdef MYMPD_ENABLE_EXPERIMENTAL
        if (mpd_connection_cmp_server_version(stickerdb->conn, 0, 24, 0) >= 0) {
            // Waits for merging of: https://github.com/MusicPlayerDaemon/MPD/pull/1895
            MYMPD_LOG_INFO(stickerdb->name, "Enabling sticker sort and window feature");
            stickerdb->mpd_state->feat.sticker_sort_window = true;
            MYMPD_LOG_INFO(stickerdb->name, "Enabling sticker value int handling feature");
            stickerdb->mpd_state->feat.sticker_int = true;
        }
    #endif
    mympd_api_request_sticker_features(stickerdb->mpd_state->feat.stickers,
        stickerdb->mpd_state->feat.sticker_sort_window, stickerdb->mpd_state->feat.sticker_int);
    MYMPD_LOG_DEBUG("stickerdb", "MPD connected and waiting for commands");
    return true;
}

/**
 * Disconnects from MPD
 * @param stickerdb pointer to stickerdb state
 */
void stickerdb_disconnect(struct t_stickerdb_state *stickerdb) {
    if (stickerdb->conn != NULL) {
        MYMPD_LOG_INFO(stickerdb->name, "Disconnecting from mpd");
        mpd_connection_free(stickerdb->conn);
    }
    stickerdb->conn = NULL;
    stickerdb->conn_state = MPD_DISCONNECTED;
}

/**
 * Discards waiting idle events for the stickerdb connection.
 * This prevents the connection to timeout.
 * @param stickerdb pointer to the stickerdb state
 */
bool stickerdb_idle(struct t_stickerdb_state *stickerdb) {
    MYMPD_LOG_DEBUG("stickerdb", "Discarding idle events");
    mympd_api_request_trigger_event_emit(TRIGGER_MPD_STICKER, MPD_PARTITION_DEFAULT);
    return stickerdb_exit_idle(stickerdb) &&
        stickerdb_enter_idle(stickerdb);
}

/**
 * Enters the idle mode
 * @param stickerdb pointer to the stickerdb state
 * @return true on success, else false
 */
bool stickerdb_enter_idle(struct t_stickerdb_state *stickerdb) {
    MYMPD_LOG_DEBUG("stickerdb", "Entering idle mode");
    // the idle events are discarded in the mympd api loop
    if (mpd_send_idle_mask(stickerdb->conn, MPD_IDLE_STICKER) == false) {
        MYMPD_LOG_ERROR("stickerdb", "Error entering idle mode");
        stickerdb_disconnect(stickerdb);
        return false;
    }
    return true;
}

/**
 * Exits the idle mode, ignoring all idle events
 * @param stickerdb pointer to the stickerdb state
 * @return true on success, else false
 */
bool stickerdb_exit_idle(struct t_stickerdb_state *stickerdb) {
    MYMPD_LOG_DEBUG("stickerdb", "Exiting idle mode");
    if (mpd_send_noidle(stickerdb->conn) == false) {
        MYMPD_LOG_ERROR("stickerdb", "Error exiting idle mode");
    }
    mpd_response_finish(stickerdb->conn);
    return stickerdb_check_error_and_recover(stickerdb, "mpd_run_noidle");
}

/**
 * Checks for an mpd error and tries to recover.
 * @param stickerdb pointer to the stickerdb state
 * @param command command to check for the error
 * @return true on success else false
 */
bool stickerdb_check_error_and_recover(struct t_stickerdb_state *stickerdb, const char *command) {
    enum mpd_error error = mpd_connection_get_error(stickerdb->conn);
    if (error != MPD_ERROR_SUCCESS) {
        const char *error_msg = mpd_connection_get_error_message(stickerdb->conn);
            
        if (error == MPD_ERROR_SERVER) {
            enum mpd_server_error server_error = mpd_connection_get_server_error(stickerdb->conn);
            MYMPD_LOG_ERROR(stickerdb->name, "MPD error for command %s: %s (%d, %d)", command, error_msg , error, server_error);
        }
        else {
            MYMPD_LOG_ERROR(stickerdb->name, "MPD error for command %s: %s (%d)", command, error_msg , error);
        }
        //try to recover from error
        if (mpd_connection_clear_error(stickerdb->conn) == false) {
            MYMPD_LOG_ERROR(stickerdb->name, "%s", "Unrecoverable MPD error");
            stickerdb->conn_state = MPD_FAILURE;
        }
        else {
            mpd_response_finish(stickerdb->conn);
        }
        return false;
    }
    return true;
}

/**
 * Gets a sticker for a song.
 * * You must manage the idle state manually.
 * @param stickerdb pointer to the stickerdb state
 * @param uri song uri
 * @param name sticker name
 * @return pointer to sticker value
 */
sds stickerdb_get_batch(struct t_stickerdb_state *stickerdb, const char *uri, const char *name) {
    if (is_streamuri(uri) == true) {
        return sdsempty();
    }
    sds value = get_sticker_value(stickerdb, uri, name);
    return value;
}

/**
 * Gets a sticker for a song
 * @param stickerdb pointer to the stickerdb state
 * @param uri song uri
 * @param name sticker name
 * @return pointer to sticker value
 */
sds stickerdb_get(struct t_stickerdb_state *stickerdb, const char *uri, const char *name) {
    if (is_streamuri(uri) == true) {
        return sdsempty();
    }
    if (stickerdb_connect(stickerdb) == false) {
        return sdsempty();
    }
    sds value = get_sticker_value(stickerdb, uri, name);
    stickerdb_enter_idle(stickerdb);
    return value;
}

/**
 * Gets an int64_t value sticker for a song
 * You must manage the idle state manually.
 * @param stickerdb pointer to the stickerdb state
 * @param uri song uri
 * @param name sticker name
 * @return pointer to sticker value
 */
int64_t stickerdb_get_int64_batch(struct t_stickerdb_state *stickerdb, const char *uri, const char *name) {
    int64_t value = 0;
    if (is_streamuri(uri) == true) {
        return value;
    }
    value = get_sticker_int64(stickerdb, uri, name);
    return value;
}

/**
 * Gets an int64_t value sticker for a song
 * @param stickerdb pointer to the stickerdb state
 * @param uri song uri
 * @param name sticker name
 * @return pointer to sticker value
 */
int64_t stickerdb_get_int64(struct t_stickerdb_state *stickerdb, const char *uri, const char *name) {
    int64_t value = 0;
    if (is_streamuri(uri) == true) {
        return value;
    }
    if (stickerdb_connect(stickerdb) == false) {
        return false;
    }
    value = get_sticker_int64(stickerdb, uri, name);
    stickerdb_enter_idle(stickerdb);
    return value;
}

/**
 * Gets all stickers for a song.
 * You must manage the idle state manually.
 * @param stickerdb pointer to the stickerdb state
 * @param uri song uri
 * @param sticker pointer to t_sticker struct to populate
 * @param user_defined get user defines stickers?
 * @return Initialized and populated sticker struct or NULL on error
 */
struct t_sticker *stickerdb_get_all_batch(struct t_stickerdb_state *stickerdb, const char *uri, struct t_sticker *sticker, bool user_defined) {
    if (is_streamuri(uri) == true) {
        return NULL;
    }
    return get_sticker_all(stickerdb, uri, sticker, user_defined);
}

/**
 * Gets all stickers for a song
 * @param stickerdb pointer to the stickerdb state
 * @param uri song uri
 * @param sticker pointer to t_sticker struct to populate
 * @param user_defined get user defines stickers?
 * @return Initialized and populated sticker struct or NULL on error
 */
struct t_sticker *stickerdb_get_all(struct t_stickerdb_state *stickerdb, const char *uri, struct t_sticker *sticker, bool user_defined) {
    if (is_streamuri(uri) == true) {
        return NULL;
    }
    if (stickerdb_connect(stickerdb) == false) {
        return NULL;
    }
    sticker = get_sticker_all(stickerdb, uri, sticker, user_defined);
    stickerdb_enter_idle(stickerdb);
    return sticker;
}

/**
 * Gets all song stickers by name
 * @param stickerdb pointer to the stickerdb state
 * @param name sticker name
 * @return newly allocated radix tree or NULL on error
 */
rax *stickerdb_find_stickers_by_name(struct t_stickerdb_state *stickerdb, const char *name) {
    return stickerdb_find_stickers_by_name_value(stickerdb, name, MPD_STICKER_OP_UNKOWN, NULL);
}

/**
 * Gets all song stickers by name and value
 * @param stickerdb pointer to the stickerdb state
 * @param name sticker name
 * @param op compare operator: MPD_STICKER_OP_EQ, MPD_STICKER_OP_GT, MPD_STICKER_OP_LT
 * @param value sticker value
 * @return newly allocated radix tree or NULL on error
 */
rax *stickerdb_find_stickers_by_name_value(struct t_stickerdb_state *stickerdb,
        const char *name, enum mpd_sticker_operator op, const char *value)
{
    if (stickerdb_connect(stickerdb) == false) {
        return NULL;
    }
    rax *stickers = raxNew();
    struct mpd_pair *pair;
    ssize_t name_len = (ssize_t)strlen(name) + 1;
    sds file = sdsempty();
    if (mpd_sticker_search_begin(stickerdb->conn, "song", NULL, name) == false ||
        sticker_search_add_value_constraint(stickerdb, op, value) == false)
    {
        mpd_sticker_search_cancel(stickerdb->conn);
        stickerdb_free_find_result(stickers);
        return NULL;
    }
    if (mpd_sticker_search_commit(stickerdb->conn) == true) {
        while ((pair = mpd_recv_pair(stickerdb->conn)) != NULL) {
            if (strcmp(pair->name, "file") == 0) {
                file = sds_replace(file, pair->value);
            }
            else if (strcmp(pair->name, "sticker") == 0) {
                sds sticker_value = sdsnew(pair->value);
                sdsrange(sticker_value, name_len, -1);
                raxInsert(stickers, (unsigned char *)file, sdslen(file), sticker_value, NULL);
            }
            mpd_return_sticker(stickerdb->conn, pair);
        }
    }
    mpd_response_finish(stickerdb->conn);
    FREE_SDS(file);
    if (stickerdb_check_error_and_recover(stickerdb, "mpd_send_sticker_list") == true) {
        stickerdb_enter_idle(stickerdb);
    }
    else {
        stickerdb_free_find_result(stickers);
        return NULL;
    }
    MYMPD_LOG_DEBUG("stickerdb", "Found %" PRIu64 " stickers for %s",
            stickers->numele, name);
    return stickers;
}

/**
 * Gets a sorted list of stickers by name and value
 * @param stickerdb pointer to the stickerdb state
 * @param name sticker name
 * @param op mpd sticker compare operator
 * @param value sticker value or NULL to get all stickers with this name
 * @param sort sticker sort type
 * @param sort_desc sort descending?
 * @param start window start (including)
 * @param end window end (excluding), use UINT_MAX for open end
 * @return struct t_list* 
 */
struct t_list *stickerdb_find_stickers_sorted(struct t_stickerdb_state *stickerdb,
        const char *name, enum mpd_sticker_operator op, const char *value,
        enum mpd_sticker_sort sort, bool sort_desc, unsigned start, unsigned end)
{
    if (stickerdb_connect(stickerdb) == false) {
        return NULL;
    }
    struct t_list *stickers = list_new();
    struct mpd_pair *pair;
    ssize_t name_len = (ssize_t)strlen(name) + 1;
    sds file = sdsempty();
    if (mpd_sticker_search_begin(stickerdb->conn, "song", NULL, name) == false ||
        sticker_search_add_value_constraint(stickerdb, op, value) == false ||
        sticker_search_add_sort(stickerdb, sort, sort_desc) == false ||
        sticker_search_add_window(stickerdb, start, end) == false)
    {
        mpd_sticker_search_cancel(stickerdb->conn);
        list_free(stickers);
        return NULL;
    }
    if (mpd_sticker_search_commit(stickerdb->conn) == true) {
        while ((pair = mpd_recv_pair(stickerdb->conn)) != NULL) {
            if (strcmp(pair->name, "file") == 0) {
                file = sds_replace(file, pair->value);
            }
            else if (strcmp(pair->name, "sticker") == 0) {
                sds sticker_value = sdsnew(pair->value);
                sdsrange(sticker_value, name_len, -1);
                list_push(stickers, file, 0, sticker_value, NULL);
                FREE_SDS(sticker_value);
            }
            mpd_return_sticker(stickerdb->conn, pair);
        }
    }
    mpd_response_finish(stickerdb->conn);
    FREE_SDS(file);
    if (stickerdb_check_error_and_recover(stickerdb, "mpd_send_sticker_list") == true) {
        stickerdb_enter_idle(stickerdb);
    }
    else {
        list_free(stickers);
        return NULL;
    }
    MYMPD_LOG_DEBUG("stickerdb", "Found %u stickers for %s", stickers->length, name);
    return stickers;
}

/**
 * Frees the sticker find result
 * @param stickers pointer to stickers rax tree
 */
void stickerdb_free_find_result(rax *stickers) {
    if (stickers == NULL) {
        return;
    }
    raxIterator iter;
    raxStart(&iter, stickers);
    raxSeek(&iter, "^", NULL, 0);
    while (raxNext(&iter)) {
        FREE_SDS(iter.data);
    }
    raxStop(&iter);
    raxFree(stickers);
}

/**
 * Sets a sticker for a song
 * @param stickerdb pointer to the stickerdb state
 * @param uri song uri
 * @param name sticker name
 * @param value sticker value
 * @return true on success, else false
 */
bool stickerdb_set(struct t_stickerdb_state *stickerdb, const char *uri, const char *name, const char *value) {
    if (is_streamuri(uri) == true) {
        return true;
    }
    if (stickerdb_connect(stickerdb) == false) {
        return false;
    }
    bool rc = set_sticker_value(stickerdb, uri, name, value);
    stickerdb_enter_idle(stickerdb);
    return rc;
}

/**
 * Sets a sticker for a song
 * @param stickerdb pointer to the stickerdb state
 * @param uri song uri
 * @param name sticker name
 * @param value sticker value
 * @return true on success, else false
 */
bool stickerdb_set_int64(struct t_stickerdb_state *stickerdb, const char *uri, const char *name, int64_t value) {
    if (is_streamuri(uri) == true) {
        return true;
    }
    if (stickerdb_connect(stickerdb) == false) {
        return false;
    }
    bool rc = set_sticker_int64(stickerdb, uri, name, value);
    stickerdb_enter_idle(stickerdb);
    return rc;
}

/**
 * Increments a sticker for a song
 * @param stickerdb pointer to the stickerdb state
 * @param uri song uri
 * @param name sticker name
 * @return true on success, else false
 */
bool stickerdb_inc(struct t_stickerdb_state *stickerdb, const char *uri, const char *name) {
    if (is_streamuri(uri) == true) {
        return true;
    }
    if (stickerdb_connect(stickerdb) == false) {
        return false;
    }
    bool rc = inc_sticker(stickerdb, uri, name);
    stickerdb_enter_idle(stickerdb);
    return rc;
}

/**
 * Sets the myMPD elapsed timestamp sticker
 * @param stickerdb pointer to the stickerdb state
 * @param uri song uri
 * @param elapsed timestamp
 * @return true on success, else false
 */
bool stickerdb_set_elapsed(struct t_stickerdb_state *stickerdb, const char *uri, time_t elapsed) {
    return stickerdb_set_int64(stickerdb, uri, sticker_name_lookup(STICKER_ELAPSED), (int64_t)elapsed);
}

/**
 * Increments a counter and sets a timestamp
 * @param stickerdb pointer to the stickerdb state
 * @param uri song uri
 * @param name_inc sticker name for counter
 * @param name_timestamp sticker name for timestamp
 * @param timestamp timestamp to set
 * @return true on success, else false
 */
bool stickerdb_inc_set(struct t_stickerdb_state *stickerdb, const char *uri,
        enum mympd_sticker_types name_inc, enum mympd_sticker_types name_timestamp, time_t timestamp)
{
    if (is_streamuri(uri) == true) {
        return true;
    }
    if (stickerdb_connect(stickerdb) == false) {
        return false;
    }
    bool rc = set_sticker_int64(stickerdb, uri, sticker_name_lookup(name_timestamp), (int64_t)timestamp) &&
        inc_sticker(stickerdb, uri, sticker_name_lookup(name_inc));
    stickerdb_enter_idle(stickerdb);
    return rc;
}

/**
 * Increments the myMPD song play count
 * @param stickerdb pointer to the stickerdb state
 * @param uri song uri
 * @param timestamp timestamp to set
 * @return true on success, else false
 */
bool stickerdb_inc_play_count(struct t_stickerdb_state *stickerdb, const char *uri, time_t timestamp) {
    return stickerdb_inc_set(stickerdb, uri, STICKER_PLAY_COUNT, STICKER_LAST_PLAYED, timestamp);
}

/**
 * Increments the myMPD song skip count and sets the
 * @param stickerdb pointer to the stickerdb state
 * @param uri song uri
 * @return true on success, else false
 */
bool stickerdb_inc_skip_count(struct t_stickerdb_state *stickerdb, const char *uri) {
    time_t timestamp = time(NULL);
    return stickerdb_inc_set(stickerdb, uri, STICKER_SKIP_COUNT, STICKER_LAST_SKIPPED, timestamp);
}

/**
 * Sets the myMPD like sticker
 * @param stickerdb pointer to the stickerdb state
 * @param uri song uri
 * @param value 0 = hate, 1 = neutral, 2 = like
 * @return true on success, else false
 */
bool stickerdb_set_like(struct t_stickerdb_state *stickerdb, const char *uri, enum sticker_like value) {
    if (value < STICKER_LIKE_MIN || value > STICKER_LIKE_MAX) {
        return false;
    }
    return stickerdb_set_int64(stickerdb, uri, sticker_name_lookup(STICKER_LIKE), (int64_t)value);
}

/**
 * Sets the myMPD rating sticker
 * @param stickerdb pointer to the stickerdb state
 * @param uri song uri
 * @param value 0 - 10 stars
 * @return true on success, else false
 */
bool stickerdb_set_rating(struct t_stickerdb_state *stickerdb, const char *uri, int value) {
    if (value < STICKER_RATING_MIN || value > STICKER_RATING_MAX) {
        return false;
    }
    return stickerdb_set_int64(stickerdb, uri, sticker_name_lookup(STICKER_RATING), (int64_t)value);
}

/**
 * Removes a sticker
 * @param stickerdb pointer to the stickerdb state
 * @param uri song uri
 * @param name sticker name
 * @return bool true on success, else false
 */
bool stickerdb_remove(struct t_stickerdb_state *stickerdb, const char *uri, const char *name) {
    if (is_streamuri(uri) == true) {
        return true;
    }
    if (stickerdb_connect(stickerdb) == false) {
        return false;
    }
    bool rc = remove_sticker(stickerdb, uri, name);
    stickerdb_enter_idle(stickerdb);
    return rc;
}

// Private functions

/**
 * Adds a mpd sticker search value constraint if value is not NULL
 * @param stickerdb pointer to the stickerdb state
 * @param op compare operator
 * @param value sticker value to search
 * @return true on success, else false
 */
static bool sticker_search_add_value_constraint(struct t_stickerdb_state *stickerdb, enum mpd_sticker_operator op, const char *value) {
    if (value != NULL) {
        return mpd_sticker_search_add_value_constraint(stickerdb->conn, op, value);
    }
    return true;
}

/**
 * Adds a mpd sticker sort definition, if supported by MPD
 * @param stickerdb pointer to the stickerdb state
 * @param sort mpd sticker sort type
 * @param desc sort descending?
 * @return true on success, else false
 */
static bool sticker_search_add_sort(struct t_stickerdb_state *stickerdb, enum mpd_sticker_sort sort, bool desc) {
    if (stickerdb->mpd_state->feat.sticker_sort_window == true &&
        sort != MPD_STICKER_SORT_UNKOWN)
    {
        return mpd_sticker_search_add_sort(stickerdb->conn, sort, desc);
    }
    return true;
}

/**
 * Adds a mpd sticker window definition, if supported by MPD
 * @param stickerdb pointer to the stickerdb state
 * @param start window start (including)
 * @param end window end (excluding)
 * @return true on success, else false
 */
static bool sticker_search_add_window(struct t_stickerdb_state *stickerdb, unsigned start, unsigned end) {
    if (stickerdb->mpd_state->feat.sticker_sort_window == true) {
        return mpd_sticker_search_add_window(stickerdb->conn, start, end);
    }
    return true;
}

/**
 * Initializes the sticker struct and gets all stickers for a song.
 * You must manage the idle state manually.
 * @param stickerdb pointer to the stickerdb state
 * @param uri song uri
 * @param sticker pointer to t_sticker struct to populate
 * @param user_defined get user defines stickers?
 * @return the initialized and populated sticker struct
 */
static struct t_sticker *get_sticker_all(struct t_stickerdb_state *stickerdb, const char *uri, struct t_sticker *sticker, bool user_defined) {
    struct mpd_pair *pair;
    sticker_struct_init(sticker);
    if (mpd_send_sticker_list(stickerdb->conn, "song", uri)) {
        while ((pair = mpd_recv_sticker(stickerdb->conn)) != NULL) {
            enum mympd_sticker_types sticker_type = sticker_name_parse(pair->name);
            if (sticker_type != STICKER_UNKNOWN) {
                int num;
                enum str2int_errno rc = str2int(&num, pair->value);
                sticker->mympd[sticker_type] = rc == STR2INT_SUCCESS
                    ? num
                    : 0;
            }
            else if (user_defined == true) {
                list_push(&sticker->user, pair->name, 0, pair->value, NULL);
            }
            mpd_return_sticker(stickerdb->conn, pair);
        }
    }
    mpd_response_finish(stickerdb->conn);
    stickerdb_check_error_and_recover(stickerdb, "mpd_send_sticker_list");
    return sticker;
}

/**
 * Gets a string value from sticker
 * @param stickerdb pointer to the stickerdb state
 * @param uri song uri
 * @param name sticker name
 * @return string
 */
static sds get_sticker_value(struct t_stickerdb_state *stickerdb, const char *uri, const char *name) {
    struct mpd_pair *pair;
    sds value = sdsempty();
    if (mpd_send_sticker_list(stickerdb->conn, "song", uri)) {
        while ((pair = mpd_recv_sticker(stickerdb->conn)) != NULL) {
            if (strcmp(pair->name, name) == 0) {
                value = sdscat(value, pair->value);
            }
            mpd_return_sticker(stickerdb->conn, pair);
        }
    }
    mpd_response_finish(stickerdb->conn);
    stickerdb_check_error_and_recover(stickerdb, "mpd_send_sticker_list");
    return value;
}

/**
 * Gets an int64t value from sticker
 * @param stickerdb pointer to the stickerdb state
 * @param uri song uri
 * @param name sticker name
 * @return number
 */
int64_t get_sticker_int64(struct t_stickerdb_state *stickerdb, const char *uri, const char *name) {
    struct mpd_pair *pair;
    int64_t value = 0;
    if (mpd_send_sticker_list(stickerdb->conn, "song", uri)) {
        while ((pair = mpd_recv_sticker(stickerdb->conn)) != NULL) {
            if (strcmp(pair->name, name) == 0) {
                str2int64(&value, pair->value);
            }
            mpd_return_sticker(stickerdb->conn, pair);
        }
    }
    mpd_response_finish(stickerdb->conn);
    stickerdb_check_error_and_recover(stickerdb, "mpd_send_sticker_list");
    return value;
}

/**
 * Sets a sticker string value
 * @param stickerdb pointer to the stickerdb state
 * @param uri song uri
 * @param name sticker name
 * @param value string to set
 * @return true on success, else false
 */
static bool set_sticker_value(struct t_stickerdb_state *stickerdb, const char *uri, const char *name, const char *value) {
    MYMPD_LOG_INFO(stickerdb->name, "Setting sticker: \"%s\" -> %s: %s", uri, name, value);
    mpd_run_sticker_set(stickerdb->conn, "song", uri, name, value);
    return stickerdb_check_error_and_recover(stickerdb, "mpd_run_sticker_set");
}

/**
 * Sets an int64_t sticker value
 * @param stickerdb pointer to the stickerdb state
 * @param uri song uri
 * @param name sticker name
 * @param value number to set
 * @return true on success, else false
 */
static bool set_sticker_int64(struct t_stickerdb_state *stickerdb, const char *uri, const char *name, int64_t value) {
    sds value_str = sdsfromlonglong((long long)value);
    bool rc = set_sticker_value(stickerdb, uri, name, value_str);
    FREE_SDS(value_str);
    return rc;
}

/**
 * Increments a sticker
 * @param stickerdb pointer to the stickerdb state
 * @param uri song uri
 * @param name sticker name
 * @return true on success, else false
 */
static bool inc_sticker(struct t_stickerdb_state *stickerdb, const char *uri, const char *name) {
    int64_t value = get_sticker_int64(stickerdb, uri, name);
    if (value < INT_MAX) {
        value++;
    }
    return set_sticker_int64(stickerdb, uri, name, value);
}

/**
 * Removes a sticker
 * @param stickerdb pointer to the stickerdb state
 * @param uri song uri
 * @param name sticker name
 * @return true on success, else false
 */
static bool remove_sticker(struct t_stickerdb_state *stickerdb, const char *uri, const char *name) {
    MYMPD_LOG_INFO(stickerdb->name, "Removing sticker: \"%s\" -> %s", uri, name);
    mpd_run_sticker_delete(stickerdb->conn, "song", uri, name);
    return stickerdb_check_error_and_recover(stickerdb, "mpd_run_sticker_delete");
}

/**
 * Creates the connection to MPD
 * @param stickerdb pointer to the stickerdb state
 * @return true on success, else false
 */
static bool stickerdb_connect_mpd(struct t_stickerdb_state *stickerdb) {
    if (stickerdb->mpd_state->mpd_host[0] == '/') {
        MYMPD_LOG_NOTICE(stickerdb->name, "Connecting to socket \"%s\"", stickerdb->mpd_state->mpd_host);
    }
    else {
        MYMPD_LOG_NOTICE(stickerdb->name, "Connecting to \"%s:%d\"", stickerdb->mpd_state->mpd_host, stickerdb->mpd_state->mpd_port);
    }
    stickerdb->conn = mpd_connection_new(stickerdb->mpd_state->mpd_host, stickerdb->mpd_state->mpd_port, stickerdb->mpd_state->mpd_timeout);
    if (stickerdb->conn == NULL) {
        MYMPD_LOG_ERROR(stickerdb->name, "Connection failed: out-of-memory");
        stickerdb->conn_state = MPD_FAILURE;
        sds buffer = jsonrpc_event(sdsempty(), JSONRPC_EVENT_MPD_DISCONNECTED);
        ws_notify(buffer, MPD_PARTITION_ALL);
        FREE_SDS(buffer);
        return false;
    }
    if (mpd_connection_get_error(stickerdb->conn) != MPD_ERROR_SUCCESS) {
        MYMPD_LOG_ERROR(stickerdb->name, "Connection: %s", mpd_connection_get_error_message(stickerdb->conn));
        sds buffer = jsonrpc_notify_phrase(sdsempty(), JSONRPC_FACILITY_MPD,
            JSONRPC_SEVERITY_ERROR, "MPD connection error: %{error}", 2,
            "error", mpd_connection_get_error_message(stickerdb->conn));
        ws_notify(buffer, MPD_PARTITION_ALL);
        FREE_SDS(buffer);
        mpd_connection_free(stickerdb->conn);
        stickerdb->conn = NULL;
        stickerdb->conn_state = MPD_FAILURE;
        return false;
    }
    if (sdslen(stickerdb->mpd_state->mpd_pass) > 0) {
        MYMPD_LOG_DEBUG(stickerdb->name, "Password set, authenticating to MPD");
        if (mpd_run_password(stickerdb->conn, stickerdb->mpd_state->mpd_pass) == false) {
            MYMPD_LOG_ERROR(stickerdb->name, "MPD connection: %s", mpd_connection_get_error_message(stickerdb->conn));
            stickerdb->conn_state = MPD_FAILURE;
            sds buffer = jsonrpc_notify_phrase(sdsempty(), JSONRPC_FACILITY_MPD,
                JSONRPC_SEVERITY_ERROR, "MPD connection error: %{error}", 2,
                "error", mpd_connection_get_error_message(stickerdb->conn));
            ws_notify(buffer, MPD_PARTITION_ALL);
            FREE_SDS(buffer);
            return false;
        }
        MYMPD_LOG_INFO(stickerdb->name, "Successfully authenticated to MPD");
    }
    else {
        MYMPD_LOG_DEBUG(stickerdb->name, "No password set");
    }

    MYMPD_LOG_NOTICE(stickerdb->name, "Connected to MPD");
    stickerdb->conn_state = MPD_CONNECTED;
    return true;
}

/**
 * Check for sticker support
 * @param stickerdb pointer to the stickerdb state
 * @return true if stickers are supported, else false
 */
static bool check_sticker_support(struct t_stickerdb_state *stickerdb) {
    bool supported = false;
    if (mpd_send_allowed_commands(stickerdb->conn) == true) {
        struct mpd_pair *pair;
        while ((pair = mpd_recv_command_pair(stickerdb->conn)) != NULL) {
            if (strcmp(pair->value, "sticker") == 0) {
                mpd_return_pair(stickerdb->conn, pair);
                supported = true;
                break;
            }
            mpd_return_pair(stickerdb->conn, pair);
        }
    }
    mpd_response_finish(stickerdb->conn);
    if (stickerdb_check_error_and_recover(stickerdb, "mpd_send_allowed_commands") == false) {
        return false;
    }
    return supported;
}
