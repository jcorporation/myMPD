/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2023 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include "compile_time.h"
#include "mpd/connection.h"
#include "mpd/idle.h"
#include "src/mpd_client/stickerdb.h"

#include "src/lib/log.h"
#include "src/lib/mympd_state.h"
#include "src/lib/sds_extras.h"
#include "src/lib/utility.h"
#include "src/mpd_client/connection.h"
#include "src/mpd_client/errorhandler.h"

#include <inttypes.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <sys/poll.h>

// Private definitions

/**
 * myMPD sticker names
 */
static const char *const mympd_sticker_names[STICKER_COUNT] = {
    [STICKER_PLAY_COUNT] = "playCount",
    [STICKER_SKIP_COUNT] = "skipCount",
    [STICKER_LIKE] = "like",
    [STICKER_LAST_PLAYED] = "lastPlayed",
    [STICKER_LAST_SKIPPED] = "lastSkipped",
    [STICKER_ELAPSED] = "elapsed"
};

static sds get_sticker_value(struct t_partition_state *partition_state, const char *uri, const char *name);
static long long get_sticker_llong(struct t_partition_state *partition_state, const char *uri, const char *name);
static bool set_sticker_value(struct t_partition_state *partition_state, const char *uri, const char *name, const char *value);
static bool set_sticker_llong(struct t_partition_state *partition_state, const char *uri, const char *name, long long value);
static bool inc_sticker(struct t_partition_state *partition_state, const char *uri, const char *name);
static bool enter_idle(struct t_partition_state *partition_state);
static bool exit_idle(struct t_partition_state *partition_state);

// Public functions

/**
 * Returns the sticker name as string
 * @param sticker enum mympd_sticker_types
 * @return const char* the sticker name
 */
const char *stickerdb_name_lookup(enum mympd_sticker_types sticker) {
    if ((unsigned)sticker >= STICKER_COUNT) {
        return NULL;
    }
    return mympd_sticker_names[sticker];
}

/**
 * Parses the sticker name
 * @param name sticker name
 * @return enum mpd_tag_type the sticker enum
 */
enum mympd_sticker_types stickerdb_name_parse(const char *name) {
    if (name == NULL) {
        return STICKER_UNKNOWN;
    }
    for (unsigned i = 0; i < STICKER_COUNT; ++i) {
        if (strcmp(name, mympd_sticker_names[i]) == 0) {
            return (enum mympd_sticker_types)i;
        }
    }
    return STICKER_UNKNOWN;
}

/**
 * This function connects to mpd sticker instance on demand
 * or exits the idle mode
 * @param partition_state pointer to the partition state
 */
bool stickerdb_connect(struct t_partition_state *partition_state) {
    if (partition_state->conn_state == MPD_FAILURE) {
        MYMPD_LOG_DEBUG("stickerdb", "Disconnecting from MPD");
        mpd_client_disconnect_silent(partition_state, MPD_DISCONNECTED);
    }
    if (partition_state->conn_state == MPD_CONNECTED) {
        // already connected
        MYMPD_LOG_DEBUG("stickerdb", "Connected, leaving idle mode");
        if (exit_idle(partition_state) == true) {
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
    MYMPD_LOG_DEBUG("stickerdb", "MPD connected and waiting for commands");
    return true;
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
        return exit_idle(partition_state) &&
            enter_idle(partition_state);
    }
    return true;
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
    enter_idle(partition_state);
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
    enter_idle(partition_state);
    return value;
}

/**
 * Gets the last played timestamp
 * @param partition_state pointer to the partition state
 * @param uri song uri
 * @return last played timestamp
 */
time_t stickerdb_get_last_played(struct t_partition_state *partition_state, const char *uri) {
    return (time_t)stickerdb_get_llong(partition_state, uri, stickerdb_name_lookup(STICKER_LAST_PLAYED));
}

void sticker_struct_init(struct t_sticker *sticker) {
    sticker->play_count = 0;
    sticker->skip_count = 0;
    sticker->last_played = 0;
    sticker->last_skipped = 0;
    sticker->like = 1;
    sticker->elapsed = 0;
    list_init(&sticker->stickers);
}

void sticker_struct_clear(struct t_sticker *sticker) {
    list_clear(&sticker->stickers);
}

/**
 * Gets all stickers for a song
 * @param partition_state pointer to the partition state
 * @param uri song uri
 * @param sticker pointer to t_sticker struct to populate
 * @param user_defined get user defines stickers?
 * @return pointer to the list of stickers
 */
bool stickerdb_get_all(struct t_partition_state *partition_state, const char *uri, struct t_sticker *sticker, bool user_defined) {
    struct mpd_pair *pair;
    if (is_streamuri(uri) == true) {
        return false;
    }
    if (stickerdb_connect(partition_state) == false) {
        return false;
    }
    
    sticker_struct_init(sticker);

    if (mpd_send_sticker_list(partition_state->conn, "song", uri)) {
        while ((pair = mpd_recv_sticker(partition_state->conn)) != NULL) {
            enum mympd_sticker_types sticker_type = stickerdb_name_parse(pair->name);
            switch(sticker_type) {
                case STICKER_PLAY_COUNT:
                    sticker->play_count = (long)strtoimax(pair->value, NULL, 10);
                    break;
                case STICKER_SKIP_COUNT:
                    sticker->skip_count = (long)strtoimax(pair->value, NULL, 10);
                    break;
                case STICKER_LAST_PLAYED:
                    sticker->last_played = (time_t)strtoimax(pair->value, NULL, 10);
                    break;
                case STICKER_LAST_SKIPPED:
                    sticker->last_skipped = (time_t)strtoimax(pair->value, NULL, 10);
                    break;
                case STICKER_LIKE:
                    sticker->like = (int)strtoimax(pair->value, NULL, 10);
                    break;
                case STICKER_ELAPSED:
                    sticker->elapsed = (time_t)strtoimax(pair->value, NULL, 10);
                    break;
                default:
                    if (user_defined == true) {
                        list_push(&sticker->stickers, pair->name, 0, pair->value, NULL);
                    }
            }
            mpd_return_sticker(partition_state->conn, pair);
        }
    }
    mpd_response_finish(partition_state->conn);
    mympd_check_error_and_recover(partition_state, NULL, "mpd_send_sticker_list");
    enter_idle(partition_state);
    return true;
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
    enter_idle(partition_state);
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
    enter_idle(partition_state);
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
    enter_idle(partition_state);
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
    return stickerdb_set_llong(partition_state, uri, stickerdb_name_lookup(STICKER_ELAPSED), (long long)elapsed);
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
    bool rc = set_sticker_llong(partition_state, uri, stickerdb_name_lookup(name_timestamp), (long long)timestamp) &&
        inc_sticker(partition_state, uri, stickerdb_name_lookup(name_inc));
    enter_idle(partition_state);
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
    return stickerdb_set_llong(partition_state, uri, stickerdb_name_lookup(STICKER_LIKE), (long long)value);
}

// Private functions

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
 * Enters the idle mode
 * @param partition_state pointer to the partition state
 * @return true on success, else false
 */
static bool enter_idle(struct t_partition_state *partition_state) {
    MYMPD_LOG_DEBUG("stickerdb", "Entering idle mode");
    // we use MPD_IDLE_MOUNT because it is seldomly emitted
    // setting no idle event is not supported
    // the idle events are discarded in the mympd api loop
    if (mpd_send_idle_mask(partition_state->conn, MPD_IDLE_MOUNT) == false) {
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
static bool exit_idle(struct t_partition_state *partition_state) {
    MYMPD_LOG_DEBUG("stickerdb", "Exiting idle mode");
    if (mpd_send_noidle(partition_state->conn) == false) {
        MYMPD_LOG_ERROR("stickerdb", "Error exiting idle mode");
    }
    mpd_response_finish(partition_state->conn);
    return mympd_check_error_and_recover(partition_state, NULL, "mpd_run_noidle");
}
