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
#include "src/mympd_api/trigger.h"

#include <inttypes.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <sys/poll.h>

// Private definitions

static struct t_sticker *get_sticker_all(struct t_partition_state *partition_state, const char *uri, struct t_sticker *sticker, bool user_defined);
static sds get_sticker_value(struct t_partition_state *partition_state, const char *uri, const char *name);
static long long get_sticker_llong(struct t_partition_state *partition_state, const char *uri, const char *name);
static bool set_sticker_value(struct t_partition_state *partition_state, const char *uri, const char *name, const char *value);
static bool set_sticker_llong(struct t_partition_state *partition_state, const char *uri, const char *name, long long value);
static bool inc_sticker(struct t_partition_state *partition_state, const char *uri, const char *name);

// Public functions

/**
 * This function connects to mpd sticker instance on demand
 * or exits the idle mode
 * @param partition_state pointer to the partition state
 */
bool stickerdb_connect(struct t_partition_state *partition_state) {
    if (partition_state->mympd_state->config->stickers == false) {
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
    if (mpd_client_connect(partition_state, false) == false) {
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
    // check for sticker support
    partition_state->mpd_state->feat_stickers = false;
    partition_state->mympd_state->mpd_state->feat_stickers = false;
    if (mpd_send_allowed_commands(partition_state->conn) == true) {
        struct mpd_pair *pair;
        while ((pair = mpd_recv_command_pair(partition_state->conn)) != NULL) {
            if (strcmp(pair->value, "sticker") == 0) {
                MYMPD_LOG_DEBUG("stickerdb", "MPD supports stickers");
                mpd_return_pair(partition_state->conn, pair);
                // set feature flag also on shared mpd state
                partition_state->mpd_state->feat_stickers = true;
                partition_state->mympd_state->mpd_state->feat_stickers = true;
                break;
            }
            mpd_return_pair(partition_state->conn, pair);
        }
    }
    mpd_response_finish(partition_state->conn);
    if (mympd_check_error_and_recover(partition_state, NULL, "mpd_send_allowed_commands") == true) {
        if (partition_state->mpd_state->feat_stickers == false) {
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
        mympd_api_trigger_execute(&partition_state->mympd_state->trigger_list, TRIGGER_MPD_STICKER, partition_state->name);
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
 * @return newly allocated radix tree
 */
rax *stickerdb_find_stickers_by_name(struct t_partition_state *partition_state, const char *name) {
    rax *stickers = raxNew();
    if (stickerdb_connect(partition_state) == false) {
        return false;
    }
    struct mpd_pair *pair;
    ssize_t name_len = (ssize_t)strlen(name);
    name_len++;
    sds file = sdsempty();
    if (mpd_send_sticker_find(partition_state->conn, "song", "", name) == true) {
        while ((pair = mpd_recv_sticker(partition_state->conn)) != NULL) {
            if (strcmp(pair->name, "file") == 0) {
                file = sds_replace(file, pair->value);
            }
            else if (strcmp(pair->name, "sticker") == 0) {
                sds value = sdsnew(pair->value);
                sdsrange(value, name_len, -1);
                raxInsert(stickers, (unsigned char *)file, sdslen(file), value, NULL);
            }
            mpd_return_sticker(partition_state->conn, pair);
        }
    }
    mpd_response_finish(partition_state->conn);
    FREE_SDS(file);
    if (mympd_check_error_and_recover(partition_state, NULL, "mpd_send_sticker_list") == true) {
        stickerdb_enter_idle(partition_state);
    }
    return stickers;
}

void stickerdb_free_find_result(rax *stickers) {
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

// Private functions

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
