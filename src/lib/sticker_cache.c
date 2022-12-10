/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2022 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include "compile_time.h"
#include "dist/sds/sds.h"
#include "src/lib/jsonrpc.h"
#include "src/lib/list.h"
#include "src/lib/sticker_cache.h"

#include "src/lib/filehandler.h"
#include "src/lib/log.h"
#include "src/lib/mem.h"
#include "src/lib/sds_extras.h"
#include "src/lib/utility.h"
#include "src/lib/validate.h"
#include "src/mpd_client/errorhandler.h"
#include "sticker_cache.h"

#include <errno.h>
#include <string.h>

/**
 * Private definitions
 */

static struct t_sticker_type *sticker_type_new(enum mympd_sticker_types sticker_type);
static bool sticker_inc(struct t_cache *sticker_cache, struct t_partition_state *partition_state, 
        const char *uri, enum mympd_sticker_types sticker_type, long value);
static bool sticker_set(struct t_cache *sticker_cache, struct t_partition_state *partition_state,
        const char *uri, enum mympd_sticker_types sticker_type, long long value);
static sds sticker_to_cache_line(sds buffer, const char *uri, size_t uri_len, struct t_sticker *sticker);
static struct t_sticker *sticker_from_cache_line(sds line, sds *uri);

static const char *const mympd_sticker_names[STICKER_COUNT] = {
    [STICKER_PLAY_COUNT] = "playCount",
    [STICKER_SKIP_COUNT] = "skipCount",
    [STICKER_LIKE] = "like",
    [STICKER_LAST_PLAYED] = "lastPlayed",
    [STICKER_LAST_SKIPPED] = "lastSkipped",
    [STICKER_ELAPSED] = "elapsed",
    [STICKER_UNKNOWN] = NULL
};

/**
 * Public functions
 */

/**
 * Removes the sticker cache file
 * @param cachedir myMPD cache directory
 * @return bool true on success, else false
 */
bool sticker_cache_remove(sds cachedir) {
    sds filepath = sdscatfmt(sdsempty(), "%S/%s", cachedir, FILENAME_ALBUMCACHE);
    int rc = try_rm_file(filepath);
    FREE_SDS(filepath);
    return rc == RM_FILE_ERROR ? false : true;
}

/**
 * Reads the sticker cache from disc
 * @param sticker_cache pointer to t_cache struct
 * @param cachedir myMPD cache directory
 * @return bool bool true on success, else false
 */
bool sticker_cache_read(struct t_cache *sticker_cache, sds cachedir) {
    #ifdef MYMPD_DEBUG
        MEASURE_INIT
        MEASURE_START
    #endif
    sticker_cache->building = true;
    sds filepath = sdscatfmt(sdsempty(), "%S/%s", cachedir, FILENAME_STICKERCACHE);
    errno = 0;
    FILE *fp = fopen(filepath, OPEN_FLAGS_READ);
    if (fp == NULL) {
        MYMPD_LOG_DEBUG("Can not open file \"%s\"", filepath);
        if (errno != ENOENT) {
            //ignore missing last_played file
            MYMPD_LOG_ERRNO(errno);
        }
        FREE_SDS(filepath);
        sticker_cache->building = false;
        return false;
    }
    sds line = sdsempty();
    if (sticker_cache->cache == NULL) {
        sticker_cache->cache = raxNew();
    }
    while (sds_getline(&line, fp, LINE_LENGTH_MAX) == 0) {
        if (validate_json_object(line) == true) {
            sds uri = NULL;
            struct t_sticker *sticker = sticker_from_cache_line(line, &uri);
            if (sticker != NULL) {
                raxInsert(sticker_cache->cache, (unsigned char *)uri, sdslen(uri), sticker, NULL);
            }
            else {
                MYMPD_LOG_ERROR("Can not allocate memory for sticker cache");
            }
            FREE_SDS(uri);
        }
        else {
            MYMPD_LOG_ERROR("Reading sticker cache line failed");
            MYMPD_LOG_DEBUG("Erroneous line: %s", line);
        }
    }
    FREE_SDS(line);
    (void) fclose(fp);
    FREE_SDS(filepath);
    sticker_cache->building = false;
    MYMPD_LOG_INFO("Read %lld sticker struct(s) from disc", (long long)sticker_cache->cache->numele);
    if (sticker_cache->cache->numele == 0) {
        sticker_cache_remove(cachedir);
        sticker_cache_free(sticker_cache);
    }
    #ifdef MYMPD_DEBUG
        MEASURE_END
        MEASURE_PRINT("Sticker cache read");
    #endif
    return true;
}

/**
 * Returns the sticker name as string
 * @param sticker enum mympd_sticker_types
 * @return const char* the sticker name
 */
const char *sticker_name_lookup(enum mympd_sticker_types sticker) {
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
enum mympd_sticker_types sticker_name_parse(const char *name) {
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
 * Saves the sticker cache to disc as a njson file
 * @param sticker_cache pointer to t_cache struct
 * @param cachedir myMPD cache directory
 * @param free_data true=free the album cache, else not
 * @return bool true on success, else false
 */
bool sticker_cache_write(struct t_cache *sticker_cache, sds cachedir, bool free_data) {
    if (sticker_cache->cache == NULL) {
        MYMPD_LOG_DEBUG("Sticker cache is NULL not saving anything");
        return true;
    }
    MYMPD_LOG_INFO("Saving sticker cache");
    raxIterator iter;
    raxStart(&iter, sticker_cache->cache);
    raxSeek(&iter, "^", NULL, 0);
    sds tmp_file = sdscatfmt(sdsempty(), "%S/%s.XXXXXX", cachedir, FILENAME_STICKERCACHE);
    FILE *fp = open_tmp_file(tmp_file);
    if (fp == NULL) {
        FREE_SDS(tmp_file);
        return false;
    }
    bool write_rc = true;
    sds line = sdsempty();

    while (raxNext(&iter)) {
        sdsclear(line);
        line = sticker_to_cache_line(line, (char *)iter.key, iter.key_len, (struct t_sticker *)iter.data);
        if (fputs(line, fp) == EOF) {
            write_rc = false;
        }
        if (free_data == true) {
            FREE_PTR(iter.data);
        }
    }
    FREE_SDS(line);
    raxStop(&iter);
    if (free_data == true) {
        raxFree(sticker_cache->cache);
        sticker_cache->cache = NULL;
    }
    sds filepath = sdscatfmt(sdsempty(), "%S/%s", cachedir, FILENAME_STICKERCACHE);
    bool rc = rename_tmp_file(fp, tmp_file, filepath, write_rc);
    FREE_SDS(tmp_file);
    FREE_SDS(filepath);
    return rc;
}

/** Gets the sticker struct from sticker cache
 * @param sticker_cache pointer to sticker cache
 * @param uri song uri
 * @return pointer to the sticker struct
 */
struct t_sticker *get_sticker_from_cache(struct t_cache *sticker_cache, const char *uri) {
    //ignore stream uris
    if (is_streamuri(uri) == true) {
        return NULL;
    }
    //check for uninitialized sticker cache
    if (sticker_cache->cache == NULL) {
        return NULL;
    }
    //try to get sticker
    void *data = raxFind(sticker_cache->cache, (unsigned char*)uri, strlen(uri));
    if (data == raxNotFound) {
        MYMPD_LOG_ERROR("Sticker for uri \"%s\" not found in cache", uri);
        return NULL;
    }
    return (struct t_sticker *) data;
}

/**
 * Frees the sticker cache
 * @param sticker_cache pointer to t_cache struct
 */
void sticker_cache_free(struct t_cache *sticker_cache) {
    if (sticker_cache->cache == NULL) {
        MYMPD_LOG_DEBUG("Sticker cache is NULL not freeing anything");
        return;
    }
    MYMPD_LOG_DEBUG("Freeing sticker cache");
    raxIterator iter;
    raxStart(&iter, sticker_cache->cache);
    raxSeek(&iter, "^", NULL, 0);
    while (raxNext(&iter)) {
        FREE_PTR(iter.data);
    }
    raxStop(&iter);
    raxFree(sticker_cache->cache);
    sticker_cache->cache = NULL;
}

/**
 * Increments the play count sticker by one
 * @param sticker_queue pointer to sticker queue
 * @param uri song uri
 * @return true on success else false
 */
bool sticker_inc_play_count(struct t_list *sticker_queue, const char *uri) {
    if (is_streamuri(uri) == true) {
        return true;
    }
    struct t_sticker_type *sticker_type = sticker_type_new(STICKER_PLAY_COUNT);
    return list_push(sticker_queue, uri, 1, NULL, sticker_type);
}

/**
 * Increments the skip count sticker by one
 * @param sticker_queue pointer to sticker queue
 * @param uri song uri
 * @return true on success else false
 */
bool sticker_inc_skip_count(struct t_list *sticker_queue, const char *uri) {
    if (is_streamuri(uri) == true) {
        return true;
    }
    struct t_sticker_type *sticker_type = sticker_type_new(STICKER_SKIP_COUNT);
    return list_push(sticker_queue, uri, 1, NULL, sticker_type);
}

/**
 * Sets the like sticker value
 * @param sticker_queue pointer to sticker queue
 * @param uri song uri
 * @param value 0 = hate, 1 = neutral, 2 = like
 * @return true on success else false
 */
bool sticker_set_like(struct t_list *sticker_queue, const char *uri, int value) {
    if (is_streamuri(uri) == true) {
        return true;
    }
    struct t_sticker_type *sticker_type = sticker_type_new(STICKER_LIKE);
    return list_push(sticker_queue, uri, value, NULL, sticker_type);
}

/**
 * Sets the last played time sticker
 * @param sticker_queue pointer to sticker queue
 * @param uri song uri
 * @param song_start_time start time of song
 * @return true on success else false
 */
bool sticker_set_last_played(struct t_list *sticker_queue, const char *uri, time_t song_start_time) {
    if (is_streamuri(uri) == true) {
        return true;
    }
    struct t_sticker_type *sticker_type = sticker_type_new(STICKER_LAST_PLAYED);
    return list_push(sticker_queue, uri, (long long)song_start_time, NULL, sticker_type);
}

/**
 * Sets the last skipped time sticker
 * @param sticker_queue pointer to sticker queue
 * @param uri song uri
 * @return true on success else false
 */
bool sticker_set_last_skipped(struct t_list *sticker_queue, const char *uri) {
    if (is_streamuri(uri) == true) {
        return true;
    }
    time_t now = time(NULL);
    struct t_sticker_type *sticker_type = sticker_type_new(STICKER_LAST_SKIPPED);
    return list_push(sticker_queue, uri, (long long)now, NULL, sticker_type);
}

/**
 * Sets the elapsed sticker
 * @param sticker_queue pointer to sticker queue
 * @param uri song uri
 * @param elapsed elapsed time of the song
 * @return true on success else false
 */
bool sticker_set_elapsed(struct t_list *sticker_queue, const char *uri, time_t elapsed) {
    if (is_streamuri(uri) == true) {
        return true;
    }
    struct t_sticker_type *sticker_type = sticker_type_new(STICKER_ELAPSED);
    return list_push(sticker_queue, uri, (long long)elapsed, NULL, sticker_type);
}

/**
 * Shifts through the sticker queue
 * @param sticker_queue pointer to sticker queue struct
 * @param sticker_cache pointer to sticker cache struct
 * @param partition_state pointer to partition specific states
 * @return true on success else false
 */
bool sticker_dequeue(struct t_list *sticker_queue, struct t_cache *sticker_cache, struct t_partition_state *partition_state) {
    if (sticker_cache->cache == NULL ||
        sticker_cache->building == true)
    {
        //sticker cache is currently (re-)building in the mpd_worker thread
        //cache sticker write calls
        MYMPD_LOG_INFO("Delay setting stickers, sticker_cache is building");
        return false;
    }

    struct t_list_node *current;
    while ((current = list_shift_first(sticker_queue)) != NULL) {
        struct t_sticker_type *st = (struct t_sticker_type *)current->user_data;
        MYMPD_LOG_DEBUG("Setting %s = %lld for \"%s\"", sticker_name_lookup(st->sticker_type), current->value_i, current->key);
        switch(st->sticker_type) {
            case STICKER_PLAY_COUNT:
            case STICKER_SKIP_COUNT:
                sticker_inc(sticker_cache, partition_state, current->key, st->sticker_type, (long)current->value_i);
                break;
            default:
                sticker_set(sticker_cache, partition_state, current->key, st->sticker_type, current->value_i);
        }
        list_node_free_user_data(current, list_free_cb_ptr_user_data);
    }
    return true;
}

/**
 * Print the sticker struct as json list
 * @param buffer already allocated sds string to append the list
 * @param sticker pointer to sticker struct to print
 * @return pointer to the modified buffer
 */
sds sticker_cache_print_sticker(sds buffer, struct t_sticker *sticker) {
    if (sticker != NULL) {
        buffer = tojson_long(buffer, "stickerPlayCount", sticker->play_count, true);
        buffer = tojson_long(buffer, "stickerSkipCount", sticker->skip_count, true);
        buffer = tojson_long(buffer, "stickerLike", sticker->like, true);
        buffer = tojson_llong(buffer, "stickerLastPlayed", (long long)sticker->last_played, true);
        buffer = tojson_llong(buffer, "stickerLastSkipped", (long long)sticker->last_skipped, true);
        buffer = tojson_llong(buffer, "stickerElapsed", (long long)sticker->elapsed, false);
    }
    else {
        buffer = tojson_long(buffer, "stickerPlayCount", 0, true);
        buffer = tojson_long(buffer, "stickerSkipCount", 0, true);
        buffer = tojson_long(buffer, "stickerLike", 1, true);
        buffer = tojson_long(buffer, "stickerLastPlayed", 0, true);
        buffer = tojson_long(buffer, "stickerLastSkipped", 0, true);
        buffer = tojson_llong(buffer, "stickerElapsed", 0, false);
    }
    return buffer;
}

//private functions

/**
 * Creates a new t_sticker_type struct
 * @param sticker_type the sticker type
 * @return struct t_sticker_type* pointer to malloced struct
 */
static struct t_sticker_type *sticker_type_new(enum mympd_sticker_types sticker_type) {
    struct t_sticker_type *st = malloc_assert(sizeof(struct t_sticker_type));
    st->sticker_type = sticker_type;
    return st;
}

/**
 * Increments a sticker by one in the cache and the mpd sticker database
 * @param sticker_cache pointer to sticker cache struct
 * @param partition_state pointer to partition specific states
 * @param uri song uri
 * @param sticker_type mympd_sticker_types enum
 * @param value value to increment by
 * @return true on success else false
 */
static bool sticker_inc(struct t_cache *sticker_cache, struct t_partition_state *partition_state,
        const char *uri, enum mympd_sticker_types sticker_type, long value)
{
    struct t_sticker *sticker = get_sticker_from_cache(sticker_cache, uri);
    if (sticker == NULL) {
        return false;
    }
    //update sticker cache
    long new_value = 0;
    const char *sticker_str = sticker_name_lookup(sticker_type);
    switch(sticker_type) {
        case STICKER_PLAY_COUNT:
            if (sticker->play_count + value > STICKER_PLAY_COUNT_MAX) {
                sticker->play_count = STICKER_PLAY_COUNT_MAX;
            }
            else {
                sticker->play_count += value;
            }
            new_value = sticker->play_count;
            break;
        case STICKER_SKIP_COUNT:
            if (sticker->skip_count + value > STICKER_SKIP_COUNT_MAX) {
                sticker->skip_count = STICKER_SKIP_COUNT_MAX;
            }
            else {
                sticker->skip_count += value;
            }
            new_value = sticker->skip_count;
            break;
        default:
           MYMPD_LOG_ERROR("Invalid sticker name \"%s\" (%d)", sticker_str, sticker_type);
           return false;
    }

    //update mpd sticker
    sds value_str = sdsfromlonglong((long long)new_value);
    MYMPD_LOG_INFO("Setting sticker: \"%s\" -> %s: %s", uri, sticker_str, value_str);
    bool rc = mpd_run_sticker_set(partition_state->conn, "song", uri, sticker_str, value_str);
    FREE_SDS(value_str);
    return mympd_check_rc_error_and_recover(partition_state, rc, "mpd_run_sticker_set");
}

/**
 * Sets a sticker in the cache and the mpd sticker database
 * @param sticker_cache pointer to sticker cache struct
 * @param partition_state pointer to partition specific states
 * @param uri song uri
 * @param sticker_type mympd_sticker_types enum
 * @param value value to set
 * @return true on success else false
 */
static bool sticker_set(struct t_cache *sticker_cache, struct t_partition_state *partition_state,
        const char *uri, enum mympd_sticker_types sticker_type, long long value)
{
    //update sticker cache
    struct t_sticker *sticker = get_sticker_from_cache(sticker_cache, uri);
    if (sticker == NULL) {
        MYMPD_LOG_ERROR("Sticker for \"%s\" not found in cache", uri);
        return false;
    }
    const char *sticker_str = sticker_name_lookup(sticker_type);
    switch(sticker_type) {
        case STICKER_LIKE:
            sticker->like = (long)value;
            break;
        case STICKER_LAST_PLAYED:
            sticker->last_played = (time_t)value;
            break;
        case STICKER_LAST_SKIPPED:
            sticker->last_skipped = (time_t)value;
            break;
        case STICKER_ELAPSED:
            sticker->elapsed = (time_t)value;
            break;
        default:
            MYMPD_LOG_ERROR("Invalid sticker name \"%s\" (%d)", sticker_str, sticker_type);
            return false;
    }

    //update mpd sticker
    sds value_str = sdsfromlonglong(value);
    MYMPD_LOG_INFO("Setting sticker: \"%s\" -> %s: %s", uri, sticker_str, value_str);
    bool rc = mpd_run_sticker_set(partition_state->conn, "song", uri, sticker_str, value_str);
    FREE_SDS(value_str);
    return mympd_check_rc_error_and_recover(partition_state, rc, "mpd_run_sticker_set");
}

/**
 * Prints a sticker struct for the cache
 * @param buffer already allocated sds string to append
 * @param uri mpd song uri
 * @param uri_len mpd song uri length
 * @param sticker pointer to sticker struct
 * @return sds pointer to buffer
 */
static sds sticker_to_cache_line(sds buffer, const char *uri, size_t uri_len, struct t_sticker *sticker) {
    buffer = sdscatlen(buffer, "{", 1);
    buffer = tojson_char_len(buffer, "uri", (char *)uri, uri_len, true);
    buffer = tojson_long(buffer, "pc", sticker->play_count, true);
    buffer = tojson_long(buffer, "sc", sticker->skip_count, true);
    buffer = tojson_long(buffer, "li", sticker->like, true);
    buffer = tojson_llong(buffer, "lp", (long long)sticker->last_played, true);
    buffer = tojson_llong(buffer, "ls", (long long)sticker->last_skipped, true);
    buffer = tojson_llong(buffer, "el", (long long)sticker->elapsed, false);
    buffer = sdscatlen(buffer, "}\n", 2);
    return buffer;
}

/**
 * Creates a sticker struct from cache
 * @param line json line to parse
 * @param uri mpd song uri for the sticker
 * @return struct t_sticker* allocated t_sticker struct
 */
static struct t_sticker *sticker_from_cache_line(sds line, sds *uri) {
    struct t_sticker *sticker = malloc_assert(sizeof(struct t_sticker));
    sds error = sdsempty();
    if (json_get_string(line, "$.uri", 1, FILEPATH_LEN_MAX, uri, vcb_isfilepath, &error) == false ||
        json_get_long_max(line, "$.pc", &sticker->play_count, &error) == false ||
        json_get_long_max(line, "$.sc", &sticker->skip_count, &error) == false ||
        json_get_time_max(line, "$.lp", &sticker->last_played, &error) == false ||
        json_get_time_max(line, "$.ls", &sticker->last_skipped, &error) == false ||
        json_get_time_max(line, "$.el", &sticker->elapsed, &error) == false ||
        json_get_long_max(line, "$.li", &sticker->like, &error) == false)
    {
        FREE_PTR(sticker);
        sticker = NULL;
    }
    FREE_SDS(error);
    return sticker;
}
