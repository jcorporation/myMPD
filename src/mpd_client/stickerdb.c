/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2023 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include "compile_time.h"
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

static bool stickerdb_enter_idle(struct t_partition_state *partition_state);
static bool stickerdb_exit_idle(struct t_partition_state *partition_state);

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
        mpd_client_disconnect(partition_state, MPD_DISCONNECTED);
    }
    if (partition_state->conn_state == MPD_CONNECTED) {
        // already connected
        MYMPD_LOG_DEBUG("stickerdb", "Connected, leaving idle mode");
        return stickerdb_exit_idle(partition_state);
    }
    // try to connect
    MYMPD_LOG_INFO(partition_state->name, "Creating mpd connection for partition \"%s\"", partition_state->name);
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
 * Gets a sticker for a song
 * @param partition_state pointer to the partition state
 * @param uri song uri
 * @param name sticker name
 * @param idle enter idle mode?
 * @return pointer to sticker value
 */
sds stickerdb_get(struct t_partition_state *partition_state, const char *uri, const char *name, bool idle) {
    struct mpd_pair *pair;
    sds value = sdsempty();
    if (is_streamuri(uri) == true) {
        return value;
    }
    if (stickerdb_connect(partition_state) == false) {
        return false;
    }
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
    if (idle == true) {
        stickerdb_enter_idle(partition_state);
    }
    return value;
}

/**
 * Gets a long long value sticker for a song
 * @param partition_state pointer to the partition state
 * @param uri song uri
 * @param name sticker name
 * @param idle enter idle mode?
 * @return pointer to sticker value
 */
long long stickerdb_get_llong(struct t_partition_state *partition_state, const char *uri, const char *name, bool idle) {
    struct mpd_pair *pair;
    long long value = 0;
    if (is_streamuri(uri) == true) {
        return value;
    }
    if (stickerdb_connect(partition_state) == false) {
        return false;
    }
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
    if (idle == true) {
        stickerdb_enter_idle(partition_state);
    }
    return value;
}

/**
 * Gets the last played timestamp
 * @param partition_state pointer to the partition state
 * @param uri song uri
 * @return last played timestamp
 */
time_t stickerdb_get_last_played(struct t_partition_state *partition_state, const char *uri) {
    return (time_t)stickerdb_get_llong(partition_state, uri, stickerdb_name_lookup(STICKER_LAST_PLAYED), true);
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
    stickerdb_enter_idle(partition_state);
    return true;
}

/**
 * Sets a sticker for a song
 * @param partition_state pointer to the partition state
 * @param uri song uri
 * @param name sticker name
 * @param value sticker value
 * @param idle enter idle mode?
 * @return true on success, else false
 */
bool stickerdb_set(struct t_partition_state *partition_state, const char *uri, const char *name, const char *value, bool idle) {
    if (is_streamuri(uri) == true) {
        return true;
    }
    if (stickerdb_connect(partition_state) == false) {
        return false;
    }
    MYMPD_LOG_INFO(partition_state->name, "Setting sticker: \"%s\" -> %s: %s", uri, name, value);
    mpd_run_sticker_set(partition_state->conn, "song", uri, name, value);
    bool rc = mympd_check_error_and_recover(partition_state, NULL, "mpd_run_sticker_set");
    if (idle == true) {
        stickerdb_enter_idle(partition_state);
    }
    return rc;
}

/**
 * Sets a sticker for a song
 * @param partition_state pointer to the partition state
 * @param uri song uri
 * @param name sticker name
 * @param value sticker value
 * @param idle enter idle mode?
 * @return true on success, else false
 */
bool stickerdb_set_llong(struct t_partition_state *partition_state, const char *uri, const char *name, long long value, bool idle) {
    if (is_streamuri(uri) == true) {
        return true;
    }
    sds value_str = sdsfromlonglong(value);
    bool rc = stickerdb_set(partition_state, uri, name, value_str, idle);
    FREE_SDS(value_str);
    return rc;
}

/**
 * Increments a sticker for a song
 * @param partition_state pointer to the partition state
 * @param uri song uri
 * @param name sticker name
 * @param idle enter idle mode?
 * @return true on success, else false
 */
bool stickerdb_inc(struct t_partition_state *partition_state, const char *uri, const char *name, bool idle) {
    if (is_streamuri(uri) == true) {
        return true;
    }
    long long value = stickerdb_get_llong(partition_state, uri, name, true);
    if (value < INT_MAX) {
        value++;
    }
    sds value_str = sdsfromlonglong(value);
    bool rc = stickerdb_set(partition_state, uri, name, value_str, idle);
    FREE_SDS(value_str);
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
    return stickerdb_set_llong(partition_state, uri, stickerdb_name_lookup(STICKER_ELAPSED), (long long)elapsed, true);
}

/**
 * Increments the myMPD song play count
 * @param partition_state pointer to the partition state
 * @param uri song uri
 * @return true on success, else false
 */
bool stickerdb_inc_play_count(struct t_partition_state *partition_state, const char *uri, time_t timestamp) {
    return stickerdb_set_llong(partition_state, uri, stickerdb_name_lookup(STICKER_LAST_PLAYED), (long long)timestamp, true) &&
        stickerdb_inc(partition_state, uri, stickerdb_name_lookup(STICKER_PLAY_COUNT), true);
}

/**
 * Increments the myMPD song skip count and sets the
 * @param partition_state pointer to the partition state
 * @param uri song uri
 * @return true on success, else false
 */
bool stickerdb_inc_skip_count(struct t_partition_state *partition_state, const char *uri) {
    time_t timestamp = time(NULL);
    return stickerdb_set_llong(partition_state, uri, stickerdb_name_lookup(STICKER_LAST_SKIPPED), (long long)timestamp, true) &&
        stickerdb_inc(partition_state, uri, stickerdb_name_lookup(STICKER_SKIP_COUNT), true);
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
    return stickerdb_set_llong(partition_state, uri, stickerdb_name_lookup(STICKER_LIKE), (long long)value, true);
}

// Private functions

/**
 * Enters the idle mode
 * @param partition_state pointer to the partition state
 * @return true on success, else false
 */
static bool stickerdb_enter_idle(struct t_partition_state *partition_state) {
    MYMPD_LOG_DEBUG("stickerdb", "Entering idle mode");
    if (mpd_send_idle(partition_state->conn) == false) {
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
static bool stickerdb_exit_idle(struct t_partition_state *partition_state) {
    MYMPD_LOG_DEBUG("stickerdb", "Exiting idle mode");
    if (mpd_send_noidle(partition_state->conn) == false) {
        MYMPD_LOG_ERROR("stickerdb", "Error exiting idle mode");
    }
    mpd_response_finish(partition_state->conn);
    return mympd_check_error_and_recover(partition_state, NULL, "mpd_run_noidle");
}
