/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2023 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include "compile_time.h"
#include "dist/mpack/mpack.h"
#include "src/lib/sticker_cache.h"

#include "src/lib/filehandler.h"
#include "src/lib/list.h"
#include "src/lib/log.h"
#include "src/lib/mem.h"
#include "src/lib/mpack.h"
#include "src/lib/sds_extras.h"
#include "src/lib/utility.h"
#include "src/mpd_client/errorhandler.h"

#include <errno.h>
#include <inttypes.h>
#include <string.h>

/**
 * Private definitions
 */

static struct t_sticker_type *sticker_type_new(enum mympd_sticker_types sticker_type);
static bool sticker_inc(struct t_cache *sticker_cache, struct t_partition_state *partition_state, 
        const char *uri, enum mympd_sticker_types sticker_type, long value);
static bool sticker_set(struct t_cache *sticker_cache, struct t_partition_state *partition_state,
        const char *uri, enum mympd_sticker_types sticker_type, long long value);

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

/**
 * Public functions
 */

/**
 * Removes the sticker cache file
 * @param workdir myMPD working directory
 * @return bool true on success, else false
 */
bool sticker_cache_remove(sds workdir) {
    sds filepath = sdscatfmt(sdsempty(), "%S/%s/%s", workdir, DIR_WORK_TAGS, FILENAME_STICKERCACHE);
    int rc = try_rm_file(filepath);
    FREE_SDS(filepath);
    return rc == RM_FILE_ERROR ? false : true;
}

/**
 * Reads the sticker cache from disc
 * @param sticker_cache pointer to t_cache struct
 * @param workdir myMPD working directory
 * @return bool bool true on success, else false
 */
bool sticker_cache_read(struct t_cache *sticker_cache, sds workdir) {
    #ifdef MYMPD_DEBUG
        MEASURE_INIT
        MEASURE_START
    #endif
    sds filepath = sdscatfmt(sdsempty(), "%S/%s/%s", workdir, DIR_WORK_TAGS, FILENAME_STICKERCACHE);
    if (testfile_read(filepath) == false) {
        FREE_SDS(filepath);
        return false;
    }
    sticker_cache->building = true;
    sticker_cache->cache = raxNew();
    mpack_tree_t tree;
    mpack_tree_init_filename(&tree, filepath, 0);
    mpack_tree_set_error_handler(&tree, log_mpack_node_error);
    FREE_SDS(filepath);
    mpack_tree_parse(&tree);
    mpack_node_t root = mpack_tree_root(&tree);
    size_t len = mpack_node_array_length(root);
    for (size_t i = 0; i < len; i++) {
        mpack_node_t uri_node = mpack_node_array_at(root, i);
        struct t_sticker *sticker = NULL;
        mpack_node_t uri_value_node = mpack_node_map_cstr(uri_node, "uri");
        const char *uri = mpack_node_str(uri_value_node);
        size_t uri_len = mpack_node_strlen(uri_value_node);
        if (uri != NULL) {
            sticker = malloc_assert(sizeof(struct t_sticker));
            sticker->play_count = mpack_node_int(mpack_node_map_cstr(uri_node, "pc"));
            sticker->skip_count = mpack_node_int(mpack_node_map_cstr(uri_node, "sc"));
            sticker->like = mpack_node_int(mpack_node_map_cstr(uri_node, "li"));
            sticker->last_played = mpack_node_int(mpack_node_map_cstr(uri_node, "lp"));
            sticker->last_skipped = mpack_node_int(mpack_node_map_cstr(uri_node, "ls"));
            sticker->elapsed = mpack_node_int(mpack_node_map_cstr(uri_node, "el"));
            if (raxTryInsert(sticker_cache->cache, (unsigned char *)uri, uri_len, sticker, NULL) == 0) {
                MYMPD_LOG_ERROR(NULL, "Duplicate uri in sticker cache file found");
                FREE_PTR(sticker);
            }
        }
    }
    // clean up and check for errors
    bool rc = mpack_tree_destroy(&tree) != mpack_ok
        ? false
        : true;
    if (rc == false) {
        MYMPD_LOG_ERROR("default", "An error occurred decoding the data");
        sticker_cache_remove(workdir);
        sticker_cache_free(sticker_cache);
    }
    else {
        MYMPD_LOG_INFO(NULL, "Read %lld sticker struct(s) from disc", (long long)sticker_cache->cache->numele);
    }
    sticker_cache->building = false;
    #ifdef MYMPD_DEBUG
        MEASURE_END
        MEASURE_PRINT(NULL, "Sticker cache read");
    #endif
    return rc;
}

/**
 * Saves the sticker cache to disc in msgpack format
 * @param sticker_cache pointer to t_cache struct
 * @param workdir myMPD working directory
 * @param free_data true=free the album cache, else not
 * @return bool true on success, else false
 */
bool sticker_cache_write(struct t_cache *sticker_cache, sds workdir, bool free_data) {
    if (sticker_cache->cache == NULL) {
        MYMPD_LOG_DEBUG(NULL, "Sticker cache is NULL not saving anything");
        return true;
    }
    MYMPD_LOG_INFO(NULL, "Saving sticker cache to disc");
    // init mpack
    mpack_writer_t writer;
    raxIterator iter;
    raxStart(&iter, sticker_cache->cache);
    raxSeek(&iter, "^", NULL, 0);
    sds tmp_file = sdscatfmt(sdsempty(), "%S/%s/%s.XXXXXX", workdir, DIR_WORK_TAGS, FILENAME_STICKERCACHE);
    FILE *fp = open_tmp_file(tmp_file);
    if (fp == NULL) {
        FREE_SDS(tmp_file);
        return false;
    }
    mpack_writer_init_stdfile(&writer, fp, true);
    mpack_writer_set_error_handler(&writer, log_mpack_write_error);
    mpack_start_array(&writer, (uint32_t)sticker_cache->cache->numele);
    while (raxNext(&iter)) {
        struct t_sticker *sticker = (struct t_sticker *)iter.data;
        mpack_build_map(&writer);
        mpack_write_cstr(&writer, "uri");
        mpack_write_str(&writer, (char *)iter.key, (uint32_t)iter.key_len);
        mpack_write_kv(&writer, "pc", (int)sticker->play_count);
        mpack_write_kv(&writer, "sc", (int)sticker->skip_count);
        mpack_write_kv(&writer, "li", (int)sticker->like);
        mpack_write_kv(&writer, "lp", (int)sticker->last_played);
        mpack_write_kv(&writer, "ls", (int)sticker->last_skipped);
        mpack_write_kv(&writer, "el", (int)sticker->elapsed);
        if (free_data == true) {
            FREE_PTR(iter.data);
        }
        mpack_complete_map(&writer);
    }
    raxStop(&iter);
    mpack_finish_array(&writer);
    if (free_data == true) {
        raxFree(sticker_cache->cache);
        sticker_cache->cache = NULL;
    }
    // finish writing
    bool rc = mpack_writer_destroy(&writer) != mpack_ok
        ? false
        : true;
    if (rc == false) {
        rm_file(tmp_file);
        MYMPD_LOG_ERROR("default", "An error occurred encoding the data");
        FREE_SDS(tmp_file);
        return false;
    }
    // rename tmp file
    sds filepath = sdscatlen(sdsempty(), tmp_file, sdslen(tmp_file) - 7);
    errno = 0;
    if (rename(tmp_file, filepath) == -1) {
        MYMPD_LOG_ERROR(NULL, "Rename file from \"%s\" to \"%s\" failed", tmp_file, filepath);
        MYMPD_LOG_ERRNO(NULL, errno);
        rm_file(tmp_file);
        rc = false;
    }
    FREE_SDS(filepath);
    FREE_SDS(tmp_file);
    return rc;
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
        MYMPD_LOG_ERROR(NULL, "Sticker for uri \"%s\" not found in cache", uri);
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
        MYMPD_LOG_DEBUG(NULL, "Sticker cache is NULL not freeing anything");
        return;
    }
    MYMPD_LOG_DEBUG(NULL, "Freeing sticker cache");
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
    if (song_start_time == 0) {
        MYMPD_LOG_WARN(NULL, "Skip setting lastPlayed sticker");
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
    if (sticker_cache->cache == NULL) {
        MYMPD_LOG_INFO(partition_state->name, "Delay setting stickers, sticker_cache is not available");
        return false;
    }
    if (sticker_cache->building == true) {
        MYMPD_LOG_INFO(partition_state->name, "Delay setting stickers, sticker_cache is building");
        return false;
    }

    struct t_list_node *current;
    while ((current = list_shift_first(sticker_queue)) != NULL) {
        struct t_sticker_type *st = (struct t_sticker_type *)current->user_data;
        MYMPD_LOG_DEBUG(partition_state->name, "Setting %s = %lld for \"%s\"", sticker_name_lookup(st->sticker_type), current->value_i, current->key);
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
           MYMPD_LOG_ERROR(partition_state->name, "Invalid sticker type \"%s\" (%d)", sticker_str, sticker_type);
           return false;
    }

    //update mpd sticker
    sds value_str = sdsfromlonglong((long long)new_value);
    MYMPD_LOG_INFO(partition_state->name, "Setting sticker: \"%s\" -> %s: %s", uri, sticker_str, value_str);
    mpd_run_sticker_set(partition_state->conn, "song", uri, sticker_str, value_str);
    FREE_SDS(value_str);
    return mympd_check_error_and_recover(partition_state, NULL, "mpd_run_sticker_set");
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
        MYMPD_LOG_ERROR(partition_state->name, "Sticker for \"%s\" not found in cache", uri);
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
            MYMPD_LOG_ERROR(partition_state->name, "Invalid sticker name \"%s\" (%d)", sticker_str, sticker_type);
            return false;
    }

    //update mpd sticker
    sds value_str = sdsfromlonglong(value);
    MYMPD_LOG_INFO(partition_state->name, "Setting sticker: \"%s\" -> %s: %s", uri, sticker_str, value_str);
    mpd_run_sticker_set(partition_state->conn, "song", uri, sticker_str, value_str);
    FREE_SDS(value_str);
    return mympd_check_error_and_recover(partition_state, NULL, "mpd_run_sticker_set");
}
