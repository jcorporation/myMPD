/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2023 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include "compile_time.h"
#include "src/mpd_client/stickerdb.h"

#include "dist/rax/rax.h"
#include "src/lib/jsonrpc.h"
#include "src/lib/log.h"
#include "src/lib/mympd_state.h"
#include "src/lib/sds_extras.h"
#include "src/lib/sticker.h"
#include "src/lib/utility.h"
#include "src/mpd_client/connection.h"
#include "src/mpd_client/errorhandler.h"
#include "src/mympd_api/requests.h"

#include <inttypes.h>
#include <limits.h>
#include <poll.h>
#include <stdlib.h>
#include <string.h>

// Private definitions

static bool sticker_search_add_value_constraint(struct t_partition_state *partition_state, enum mpd_sticker_operator op, const char *value);
static bool sticker_search_add_sort(struct t_partition_state *partition_state, enum mpd_sticker_sort sort, bool desc);
static bool sticker_search_add_window(struct t_partition_state *partition_state, unsigned start, unsigned end);

static struct t_sticker *get_sticker_all(struct t_partition_state *partition_state, const char *uri, struct t_sticker *sticker, bool user_defined);
static sds get_sticker_value(struct t_partition_state *partition_state, const char *uri, const char *name);
static long long get_sticker_llong(struct t_partition_state *partition_state, const char *uri, const char *name);
static bool set_sticker_value(struct t_partition_state *partition_state, const char *uri, const char *name, const char *value);
static bool set_sticker_llong(struct t_partition_state *partition_state, const char *uri, const char *name, long long value);
static bool inc_sticker(struct t_partition_state *partition_state, const char *uri, const char *name);
static bool remove_sticker(struct t_partition_state *partition_state, const char *uri, const char *name);

// Public functions

/**
 * This function connects to mpd sticker instance on demand
 * or exits the idle mode
 * @param partition_state pointer to the partition state
 */
bool stickerdb_connect(struct t_partition_state *partition_state) {
    if (partition_state->config->stickers == false) {
        MYMPD_LOG_WARN("stickerdb", "Stickers are disabled by config");
        return false;
    }
    if (partition_state->conn_state == MPD_FAILURE) {
        MYMPD_LOG_DEBUG("stickerdb", "Disconnecting from MPD");
        mpd_client_disconnect_silent(partition_state, MPD_DISCONNECTED);
    }
    if (partition_state->conn_state == MPD_CONNECTED) {
        // already connected
        MYMPD_LOG_DEBUG("stickerdb", "Connected, leaving idle mode");
        if (stickerdb_exit_idle(partition_state) == true) {
            return true;
        }
        // stickerdb connection broken
        mpd_client_disconnect_silent(partition_state, MPD_DISCONNECTED);
    }
    // try to connect
    MYMPD_LOG_INFO(partition_state->name, "Creating mpd connection for %s", partition_state->name);
    if (mpd_client_connect(partition_state) == false) {
        MYMPD_LOG_DEBUG("stickerdb", "Connecting to MPD");
        return false;
    }
    // check version
    if (mpd_connection_cmp_server_version(partition_state->conn, 0, 21, 0) < 0) {
        MYMPD_LOG_DEBUG("stickerdb", "Checking version");
        MYMPD_LOG_EMERG(partition_state->name, "MPD version too old, myMPD supports only MPD version >= 0.21");
        mpd_client_disconnect_silent(partition_state, MPD_DISCONNECTED);
        return false;
    }
    partition_state->mpd_state->feat.sticker_sort_window = false;
    partition_state->mpd_state->feat.sticker_int = false;
    if (mpd_connection_cmp_server_version(partition_state->conn, 0, 24, 0) >= 0) {
        MYMPD_LOG_DEBUG(partition_state->name, "Enabling sticker sort and window feature");
        partition_state->mpd_state->feat.sticker_sort_window = true;
        MYMPD_LOG_DEBUG(partition_state->name, "Enabling sticker value int handling feature");
        partition_state->mpd_state->feat.sticker_int = true;
    }
    // check for sticker support
    partition_state->mpd_state->feat.stickers = false;
    if (mpd_send_allowed_commands(partition_state->conn) == true) {
        struct mpd_pair *pair;
        while ((pair = mpd_recv_command_pair(partition_state->conn)) != NULL) {
            if (strcmp(pair->value, "sticker") == 0) {
                MYMPD_LOG_DEBUG("stickerdb", "MPD supports stickers");
                mpd_return_pair(partition_state->conn, pair);
                partition_state->mpd_state->feat.stickers = true;
                break;
            }
            mpd_return_pair(partition_state->conn, pair);
        }
    }
    mympd_api_request_sticker_features(partition_state->mpd_state->feat.stickers,
        partition_state->mpd_state->feat.sticker_sort_window, partition_state->mpd_state->feat.sticker_int);
    mpd_response_finish(partition_state->conn);
    if (mympd_check_error_and_recover(partition_state, NULL, "mpd_send_allowed_commands") == true) {
        if (partition_state->mpd_state->feat.stickers == false) {
            MYMPD_LOG_ERROR("stickerdb", "MPD does not support stickers");
            mpd_client_disconnect_silent(partition_state, MPD_DISCONNECTED);
            send_jsonrpc_notify(JSONRPC_FACILITY_MPD, JSONRPC_SEVERITY_ERROR, MPD_PARTITION_ALL, "MPD does not support stickers");
            return false;
        }
        MYMPD_LOG_DEBUG("stickerdb", "MPD connected and waiting for commands");
        return true;
    }
    return false;
}

/**
 * Discards waiting idle events for the stickerdb connection
 * @param partition_state pointer to the partition state
 */
bool stickerdb_idle(struct t_partition_state *partition_state) {
    if (partition_state->conn == NULL ||
        partition_state->conn_state != MPD_CONNECTED)
    {
        // not connected
        return true;
    }
    struct pollfd fd[1];
    fd->fd = mpd_connection_get_fd(partition_state->conn);
    fd->events = POLLIN;
    int pollrc = poll(fd, 1, 50);
    if (pollrc < 0) {
        MYMPD_LOG_ERROR(NULL, "Error polling mpd connection");
        partition_state->conn_state = MPD_FAILURE;
        return false;
    }
    if (fd[0].revents & POLLIN) {
        // exit and reenter the idle mode to discard waiting events
        // this prevents the connection to timeout
        MYMPD_LOG_DEBUG("stickerdb", "Discarding idle events");
        mympd_api_request_trigger_event_emit(TRIGGER_MPD_STICKER, partition_state->name);
        return stickerdb_exit_idle(partition_state) &&
            stickerdb_enter_idle(partition_state);
    }
    return true;
}

/**
 * Enters the idle mode
 * @param partition_state pointer to the partition state
 * @return true on success, else false
 */
bool stickerdb_enter_idle(struct t_partition_state *partition_state) {
    MYMPD_LOG_DEBUG("stickerdb", "Entering idle mode");
    // the idle events are discarded in the mympd api loop
    if (mpd_send_idle_mask(partition_state->conn, MPD_IDLE_STICKER) == false) {
        MYMPD_LOG_ERROR("stickerdb", "Error entering idle mode");
        mpd_client_disconnect_silent(partition_state, MPD_DISCONNECTED);
        return false;
    }
    return true;
}

/**
 * Exits the idle mode, ignoring all idle events
 * @param partition_state pointer to the partition state
 * @return true on success, else false
 */
bool stickerdb_exit_idle(struct t_partition_state *partition_state) {
    MYMPD_LOG_DEBUG("stickerdb", "Exiting idle mode");
    if (mpd_send_noidle(partition_state->conn) == false) {
        MYMPD_LOG_ERROR("stickerdb", "Error exiting idle mode");
    }
    mpd_response_finish(partition_state->conn);
    return mympd_check_error_and_recover(partition_state, NULL, "mpd_run_noidle");
}

/**
 * Gets a sticker for a song.
 * * You must manage the idle state manually.
 * @param partition_state pointer to the partition state
 * @param uri song uri
 * @param name sticker name
 * @return pointer to sticker value
 */
sds stickerdb_get_batch(struct t_partition_state *partition_state, const char *uri, const char *name) {
    if (is_streamuri(uri) == true) {
        return sdsempty();
    }
    sds value = get_sticker_value(partition_state, uri, name);
    return value;
}

/**
 * Gets a sticker for a song
 * @param partition_state pointer to the partition state
 * @param uri song uri
 * @param name sticker name
 * @return pointer to sticker value
 */
sds stickerdb_get(struct t_partition_state *partition_state, const char *uri, const char *name) {
    if (is_streamuri(uri) == true) {
        return sdsempty();
    }
    if (stickerdb_connect(partition_state) == false) {
        return sdsempty();
    }
    sds value = get_sticker_value(partition_state, uri, name);
    stickerdb_enter_idle(partition_state);
    return value;
}

/**
 * Gets a long long value sticker for a song
 * You must manage the idle state manually.
 * @param partition_state pointer to the partition state
 * @param uri song uri
 * @param name sticker name
 * @return pointer to sticker value
 */
long long stickerdb_get_llong_batch(struct t_partition_state *partition_state, const char *uri, const char *name) {
    long long value = 0;
    if (is_streamuri(uri) == true) {
        return value;
    }
    value = get_sticker_llong(partition_state, uri, name);
    return value;
}

/**
 * Gets a long long value sticker for a song
 * @param partition_state pointer to the partition state
 * @param uri song uri
 * @param name sticker name
 * @return pointer to sticker value
 */
long long stickerdb_get_llong(struct t_partition_state *partition_state, const char *uri, const char *name) {
    long long value = 0;
    if (is_streamuri(uri) == true) {
        return value;
    }
    if (stickerdb_connect(partition_state) == false) {
        return false;
    }
    value = get_sticker_llong(partition_state, uri, name);
    stickerdb_enter_idle(partition_state);
    return value;
}

/**
 * Gets all stickers for a song.
 * You must manage the idle state manually.
 * @param partition_state pointer to the partition state
 * @param uri song uri
 * @param sticker pointer to t_sticker struct to populate
 * @param user_defined get user defines stickers?
 * @return Initialized and populated sticker struct or NULL on error
 */
struct t_sticker *stickerdb_get_all_batch(struct t_partition_state *partition_state, const char *uri, struct t_sticker *sticker, bool user_defined) {
    if (is_streamuri(uri) == true) {
        return NULL;
    }
    return get_sticker_all(partition_state, uri, sticker, user_defined);
}

/**
 * Gets all stickers for a song
 * @param partition_state pointer to the partition state
 * @param uri song uri
 * @param sticker pointer to t_sticker struct to populate
 * @param user_defined get user defines stickers?
 * @return Initialized and populated sticker struct or NULL on error
 */
struct t_sticker *stickerdb_get_all(struct t_partition_state *partition_state, const char *uri, struct t_sticker *sticker, bool user_defined) {
    if (is_streamuri(uri) == true) {
        return NULL;
    }
    if (stickerdb_connect(partition_state) == false) {
        return NULL;
    }
    sticker = get_sticker_all(partition_state, uri, sticker, user_defined);
    stickerdb_enter_idle(partition_state);
    return sticker;
}

/**
 * Gets all song stickers by name
 * @param partition_state pointer to the partition state
 * @param name sticker name
 * @return newly allocated radix tree or NULL on error
 */
rax *stickerdb_find_stickers_by_name(struct t_partition_state *partition_state, const char *name) {
    return stickerdb_find_stickers_by_name_value(partition_state, name, MPD_STICKER_OP_UNKOWN, NULL);
}

/**
 * Gets all song stickers by name and value
 * @param partition_state pointer to the partition state
 * @param name sticker name
 * @param op compare operator: MPD_STICKER_OP_EQ, MPD_STICKER_OP_GT, MPD_STICKER_OP_LT
 * @param value sticker value
 * @return newly allocated radix tree or NULL on error
 */
rax *stickerdb_find_stickers_by_name_value(struct t_partition_state *partition_state,
        const char *name, enum mpd_sticker_operator op, const char *value)
{
    if (stickerdb_connect(partition_state) == false) {
        return NULL;
    }
    rax *stickers = raxNew();
    struct mpd_pair *pair;
    ssize_t name_len = (ssize_t)strlen(name) + 1;
    sds file = sdsempty();
    if (mpd_sticker_search_begin(partition_state->conn, "song", NULL, name) == false ||
        sticker_search_add_value_constraint(partition_state, op, value) == false)
    {
        mpd_sticker_search_cancel(partition_state->conn);
        stickerdb_free_find_result(stickers);
        return NULL;
    }
    if (mpd_sticker_search_commit(partition_state->conn) == true) {
        while ((pair = mpd_recv_pair(partition_state->conn)) != NULL) {
            if (strcmp(pair->name, "file") == 0) {
                file = sds_replace(file, pair->value);
            }
            else if (strcmp(pair->name, "sticker") == 0) {
                sds sticker_value = sdsnew(pair->value);
                sdsrange(sticker_value, name_len, -1);
                raxInsert(stickers, (unsigned char *)file, sdslen(file), sticker_value, NULL);
            }
            mpd_return_sticker(partition_state->conn, pair);
        }
    }
    mpd_response_finish(partition_state->conn);
    FREE_SDS(file);
    if (mympd_check_error_and_recover(partition_state, NULL, "mpd_send_sticker_list") == true) {
        stickerdb_enter_idle(partition_state);
    }
    else {
        stickerdb_free_find_result(stickers);
        return NULL;
    }
    MYMPD_LOG_DEBUG("stickerdb", "Found %llu stickers for %s",
            (long long unsigned)stickers->numele, name);
    return stickers;
}

/**
 * Gets a sorted list of stickers by name and value
 * @param partition_state pointer to the partition state
 * @param name sticker name
 * @param op mpd sticker compare operator
 * @param value sticker value or NULL to get all stickers with this name
 * @param sort sticker sort type
 * @param sort_desc sort descending?
 * @param start window start (including)
 * @param end window end (excluding), use UINT_MAX for open end
 * @return struct t_list* 
 */
struct t_list *stickerdb_find_stickers_sorted(struct t_partition_state *partition_state,
        const char *name, enum mpd_sticker_operator op, const char *value,
        enum mpd_sticker_sort sort, bool sort_desc, unsigned start, unsigned end)
{
    if (stickerdb_connect(partition_state) == false) {
        return NULL;
    }
    struct t_list *stickers = list_new();
    struct mpd_pair *pair;
    ssize_t name_len = (ssize_t)strlen(name) + 1;
    sds file = sdsempty();
    if (mpd_sticker_search_begin(partition_state->conn, "song", NULL, name) == false ||
        sticker_search_add_value_constraint(partition_state, op, value) == false ||
        sticker_search_add_sort(partition_state, sort, sort_desc) == false ||
        sticker_search_add_window(partition_state, start, end) == false)
    {
        mpd_sticker_search_cancel(partition_state->conn);
        list_free(stickers);
        return NULL;
    }
    if (mpd_sticker_search_commit(partition_state->conn) == true) {
        while ((pair = mpd_recv_pair(partition_state->conn)) != NULL) {
            if (strcmp(pair->name, "file") == 0) {
                file = sds_replace(file, pair->value);
            }
            else if (strcmp(pair->name, "sticker") == 0) {
                sds sticker_value = sdsnew(pair->value);
                sdsrange(sticker_value, name_len, -1);
                list_push(stickers, file, 0, sticker_value, NULL);
                FREE_SDS(sticker_value);
            }
            mpd_return_sticker(partition_state->conn, pair);
        }
    }
    mpd_response_finish(partition_state->conn);
    FREE_SDS(file);
    if (mympd_check_error_and_recover(partition_state, NULL, "mpd_send_sticker_list") == true) {
        stickerdb_enter_idle(partition_state);
    }
    else {
        list_free(stickers);
        return NULL;
    }
    MYMPD_LOG_DEBUG("stickerdb", "Found %ld stickers for %s", stickers->length, name);
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
 * @param partition_state pointer to the partition state
 * @param uri song uri
 * @param name sticker name
 * @param value sticker value
 * @return true on success, else false
 */
bool stickerdb_set(struct t_partition_state *partition_state, const char *uri, const char *name, const char *value) {
    if (is_streamuri(uri) == true) {
        return true;
    }
    if (stickerdb_connect(partition_state) == false) {
        return false;
    }
    bool rc = set_sticker_value(partition_state, uri, name, value);
    stickerdb_enter_idle(partition_state);
    return rc;
}

/**
 * Sets a sticker for a song
 * @param partition_state pointer to the partition state
 * @param uri song uri
 * @param name sticker name
 * @param value sticker value
 * @return true on success, else false
 */
bool stickerdb_set_llong(struct t_partition_state *partition_state, const char *uri, const char *name, long long value) {
    if (is_streamuri(uri) == true) {
        return true;
    }
    if (stickerdb_connect(partition_state) == false) {
        return false;
    }
    bool rc = set_sticker_llong(partition_state, uri, name, value);
    stickerdb_enter_idle(partition_state);
    return rc;
}

/**
 * Increments a sticker for a song
 * @param partition_state pointer to the partition state
 * @param uri song uri
 * @param name sticker name
 * @return true on success, else false
 */
bool stickerdb_inc(struct t_partition_state *partition_state, const char *uri, const char *name) {
    if (is_streamuri(uri) == true) {
        return true;
    }
    if (stickerdb_connect(partition_state) == false) {
        return false;
    }
    bool rc = inc_sticker(partition_state, uri, name);
    stickerdb_enter_idle(partition_state);
    return rc;
}

/**
 * Sets the myMPD elapsed timestamp sticker
 * @param partition_state pointer to the partition state
 * @param uri song uri
 * @param elapsed timestamp
 * @return true on success, else false
 */
bool stickerdb_set_elapsed(struct t_partition_state *partition_state, const char *uri, time_t elapsed) {
    return stickerdb_set_llong(partition_state, uri, sticker_name_lookup(STICKER_ELAPSED), (long long)elapsed);
}

/**
 * Increments a counter and sets a timestamp
 * @param partition_state pointer to the partition state
 * @param uri song uri
 * @param name_inc sticker name for counter
 * @param name_timestamp sticker name for timestamp
 * @param timestamp timestamp to set
 * @return true on success, else false
 */
bool stickerdb_inc_set(struct t_partition_state *partition_state, const char *uri,
        enum mympd_sticker_types name_inc, enum mympd_sticker_types name_timestamp, time_t timestamp)
{
    if (is_streamuri(uri) == true) {
        return true;
    }
    if (stickerdb_connect(partition_state) == false) {
        return false;
    }
    bool rc = set_sticker_llong(partition_state, uri, sticker_name_lookup(name_timestamp), (long long)timestamp) &&
        inc_sticker(partition_state, uri, sticker_name_lookup(name_inc));
    stickerdb_enter_idle(partition_state);
    return rc;
}

/**
 * Increments the myMPD song play count
 * @param partition_state pointer to the partition state
 * @param uri song uri
 * @param timestamp timestamp to set
 * @return true on success, else false
 */
bool stickerdb_inc_play_count(struct t_partition_state *partition_state, const char *uri, time_t timestamp) {
    return stickerdb_inc_set(partition_state, uri, STICKER_PLAY_COUNT, STICKER_LAST_PLAYED, timestamp);
}

/**
 * Increments the myMPD song skip count and sets the
 * @param partition_state pointer to the partition state
 * @param uri song uri
 * @return true on success, else false
 */
bool stickerdb_inc_skip_count(struct t_partition_state *partition_state, const char *uri) {
    time_t timestamp = time(NULL);
    return stickerdb_inc_set(partition_state, uri, STICKER_SKIP_COUNT, STICKER_LAST_SKIPPED, timestamp);
}

/**
 * Sets the myMPD like sticker
 * @param partition_state pointer to the partition state
 * @param uri song uri
 * @param value 0 = hate, 1 = neutral, 2 = like
 * @return true on success, else false
 */
bool stickerdb_set_like(struct t_partition_state *partition_state, const char *uri, enum sticker_like value) {
    if (value < 0 || value > 2) {
        return false;
    }
    return stickerdb_set_llong(partition_state, uri, sticker_name_lookup(STICKER_LIKE), (long long)value);
}

/**
 * Sets the myMPD rating sticker
 * @param partition_state pointer to the partition state
 * @param uri song uri
 * @param value 0 - 10 stars
 * @return true on success, else false
 */
bool stickerdb_set_rating(struct t_partition_state *partition_state, const char *uri, int value) {
    if (value < 0 || value > 10) {
        return false;
    }
    return stickerdb_set_llong(partition_state, uri, sticker_name_lookup(STICKER_RATING), (long long)value);
}

/**
 * Removes a sticker
 * @param partition_state pointer to the partition state
 * @param uri song uri
 * @param name sticker name
 * @return bool true on success, else false
 */
bool stickerdb_remove(struct t_partition_state *partition_state, const char *uri, const char *name) {
    if (is_streamuri(uri) == true) {
        return true;
    }
    if (stickerdb_connect(partition_state) == false) {
        return false;
    }
    bool rc = remove_sticker(partition_state, uri, name);
    stickerdb_enter_idle(partition_state);
    return rc;
}

// Private functions

/**
 * Adds a mpd sticker search value constraint if value is not NULL
 * @param partition_state pointer to the partition state
 * @param op compare operator
 * @param value sticker value to search
 * @return true on success, else false
 */
static bool sticker_search_add_value_constraint(struct t_partition_state *partition_state, enum mpd_sticker_operator op, const char *value) {
    if (value != NULL) {
        return mpd_sticker_search_add_value_constraint(partition_state->conn, op, value);
    }
    return true;
}

/**
 * Adds a mpd sticker sort definition, if name is not NULL and supported by MPD
 * @param partition_state pointer to the partition state
 * @param sort mpd sticker sort type
 * @param desc sort descending?
 * @return true on success, else false
 */
static bool sticker_search_add_sort(struct t_partition_state *partition_state, enum mpd_sticker_sort sort, bool desc) {
    if (partition_state->mpd_state->feat.sticker_sort_window == true &&
        sort != MPD_STICKER_SORT_UNKOWN)
    {
        return mpd_sticker_search_add_sort(partition_state->conn, sort, desc);
    }
    return true;
}

/**
 * Adds a mpd sticker window definition, if supported by MPD
 * @param partition_state pointer to the partition state
 * @param start window start (including)
 * @param end window end (excluding)
 * @return true on success, else false
 */
static bool sticker_search_add_window(struct t_partition_state *partition_state, unsigned start, unsigned end) {
    if (partition_state->mpd_state->feat.sticker_sort_window == true) {
        return mpd_sticker_search_add_window(partition_state->conn, start, end);
    }
    return true;
}

/**
 * Initializes the sticker struct and gets all stickers for a song.
 * You must manage the idle state manually.
 * @param partition_state pointer to the partition state
 * @param uri song uri
 * @param sticker pointer to t_sticker struct to populate
 * @param user_defined get user defines stickers?
 * @return the initialized and populated sticker struct
 */
static struct t_sticker *get_sticker_all(struct t_partition_state *partition_state, const char *uri, struct t_sticker *sticker, bool user_defined) {
    struct mpd_pair *pair;
    sticker_struct_init(sticker);
    if (mpd_send_sticker_list(partition_state->conn, "song", uri)) {
        while ((pair = mpd_recv_sticker(partition_state->conn)) != NULL) {
            enum mympd_sticker_types sticker_type = sticker_name_parse(pair->name);
            if (sticker_type != STICKER_UNKNOWN) {
                sticker->mympd[sticker_type] = strtoll(pair->value, NULL, 10);
            }
            else if (user_defined == true) {
                list_push(&sticker->user, pair->name, 0, pair->value, NULL);
            }
            mpd_return_sticker(partition_state->conn, pair);
        }
    }
    mpd_response_finish(partition_state->conn);
    mympd_check_error_and_recover(partition_state, NULL, "mpd_send_sticker_list");
    return sticker;
}

/**
 * Gets a string value from sticker
 * @param partition_state pointer to the partition state
 * @param uri song uri
 * @param name sticker name
 * @return string
 */
static sds get_sticker_value(struct t_partition_state *partition_state, const char *uri, const char *name) {
    struct mpd_pair *pair;
    sds value = sdsempty();
    if (mpd_send_sticker_list(partition_state->conn, "song", uri)) {
        while ((pair = mpd_recv_sticker(partition_state->conn)) != NULL) {
            if (strcmp(pair->name, name) == 0) {
                value = sdscat(value, pair->value);
            }
            mpd_return_sticker(partition_state->conn, pair);
        }
    }
    mpd_response_finish(partition_state->conn);
    mympd_check_error_and_recover(partition_state, NULL, "mpd_send_sticker_list");
    return value;
}

/**
 * Gets a long long value from sticker
 * @param partition_state pointer to the partition state
 * @param uri song uri
 * @param name sticker name
 * @return number
 */
long long get_sticker_llong(struct t_partition_state *partition_state, const char *uri, const char *name) {
    struct mpd_pair *pair;
    long long value = 0;
    if (mpd_send_sticker_list(partition_state->conn, "song", uri)) {
        while ((pair = mpd_recv_sticker(partition_state->conn)) != NULL) {
            if (strcmp(pair->name, name) == 0) {
                value = strtoll(pair->value, NULL, 10);
            }
            mpd_return_sticker(partition_state->conn, pair);
        }
    }
    mpd_response_finish(partition_state->conn);
    mympd_check_error_and_recover(partition_state, NULL, "mpd_send_sticker_list");
    return value;
}

/**
 * Sets a sticker string value
 * @param partition_state pointer to the partition state
 * @param uri song uri
 * @param name sticker name
 * @param value string to set
 * @return true on success, else false
 */
static bool set_sticker_value(struct t_partition_state *partition_state, const char *uri, const char *name, const char *value) {
    MYMPD_LOG_INFO(partition_state->name, "Setting sticker: \"%s\" -> %s: %s", uri, name, value);
    mpd_run_sticker_set(partition_state->conn, "song", uri, name, value);
    return mympd_check_error_and_recover(partition_state, NULL, "mpd_run_sticker_set");
}

/**
 * Sets a long long sticker value
 * @param partition_state pointer to the partition state
 * @param uri song uri
 * @param name sticker name
 * @param value number to set
 * @return true on success, else false
 */
static bool set_sticker_llong(struct t_partition_state *partition_state, const char *uri, const char *name, long long value) {
    sds value_str = sdsfromlonglong(value);
    bool rc = set_sticker_value(partition_state, uri, name, value_str);
    FREE_SDS(value_str);
    return rc;
}

/**
 * Increments a sticker
 * @param partition_state pointer to the partition state
 * @param uri song uri
 * @param name sticker name
 * @return true on success, else false
 */
static bool inc_sticker(struct t_partition_state *partition_state, const char *uri, const char *name) {
    long long value = get_sticker_llong(partition_state, uri, name);
    if (value < INT_MAX) {
        value++;
    }
    return set_sticker_llong(partition_state, uri, name, value);
}

/**
 * Removes a sticker
 * @param partition_state  pointer to the partition state
 * @param uri song uri
 * @param name sticker name
 * @return true on success, else false
 */
static bool remove_sticker(struct t_partition_state *partition_state, const char *uri, const char *name) {
    MYMPD_LOG_INFO(partition_state->name, "Removing sticker: \"%s\" -> %s", uri, name);
    mpd_run_sticker_delete(partition_state->conn, "song", uri, name);
    return mympd_check_error_and_recover(partition_state, NULL, "mpd_run_sticker_delete");
}
