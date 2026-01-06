/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2026 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

/*! \file
 * \brief Jukebox library
 */

#include "compile_time.h"
#include "src/lib/jukebox.h"

#include "src/lib/config/mympd_state.h"
#include "src/lib/filehandler.h"
#include "src/lib/log.h"
#include "src/lib/mpack.h"
#include "src/lib/sds_extras.h"

#include <string.h>
#include <unistd.h>

/**
 * Parses the string to the jukebox mode
 * @param str string to parse
 * @return jukebox mode
 */
enum jukebox_modes jukebox_mode_parse(const char *str) {
    if (strcmp(str, "off") == 0) {
        return JUKEBOX_OFF;
    }
    if (strcmp(str, "song") == 0) {
        return JUKEBOX_ADD_SONG;
    }
    if (strcmp(str, "album") == 0) {
        return JUKEBOX_ADD_ALBUM;
    }
    if (strcmp(str, "script") == 0) {
        return JUKEBOX_SCRIPT;
    }
    return JUKEBOX_UNKNOWN;
}

/**
 * Returns the jukebox mode as string
 * @param mode the jukebox mode
 * @return jukebox mode as string
 */
const char *jukebox_mode_lookup(enum jukebox_modes mode) {
    switch (mode) {
        case JUKEBOX_OFF:
            return "off";
        case JUKEBOX_ADD_SONG:
            return "song";
        case JUKEBOX_ADD_ALBUM:
            return "album";
        case JUKEBOX_SCRIPT:
            return "script";
        case JUKEBOX_UNKNOWN:
            return NULL;
    }
    return NULL;
}

/**
 * Saves the jukebox list to disc
 * @param partition_state pointer to partition state
 * @return true on success, else false
 */
bool jukebox_file_save(struct t_partition_state *partition_state) {
    MYMPD_LOG_INFO(partition_state->name, "Saving %u jukebox entries to disc", partition_state->jukebox.queue->length);
    mpack_writer_t writer;
    sds tmp_file = sdscatfmt(sdsempty(), "%S/%S/%s.XXXXXX",
        partition_state->config->workdir, partition_state->state_dir, FILENAME_JUKEBOX);
    FILE *fp = open_tmp_file(tmp_file);
    if (fp == NULL) {
        FREE_SDS(tmp_file);
        return false;
    }
    // init mpack
    mpack_writer_init_stdfile(&writer, fp, true);
    mpack_writer_set_error_handler(&writer, log_mpack_write_error);
    mpack_build_map(&writer);
    mpack_write_kv(&writer, "jukeboxMode", partition_state->jukebox.mode);
    mpack_write_cstr(&writer, "entries");

    mpack_start_array(&writer, partition_state->jukebox.queue->length);
    struct t_list_node *current = partition_state->jukebox.queue->head;
    while (current != NULL) {
        mpack_build_map(&writer);
        mpack_write_kv(&writer, "uri", current->key);
        mpack_write_kv(&writer, "lineno", current->value_i);
        mpack_write_kv(&writer, "tagvalue", current->value_p);
        mpack_complete_map(&writer);
        current = current->next;
    }
    mpack_finish_array(&writer);
    mpack_complete_map(&writer);
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
 * Reads the jukebox list from disc
 * @param partition_state pointer to partition state
 * @return true on success, else false
 */
bool jukebox_file_read(struct t_partition_state *partition_state) {
    sds filepath = sdscatfmt(sdsempty(), "%S/%s/%s",
        partition_state->config->workdir, partition_state->state_dir, FILENAME_JUKEBOX);
    if (testfile_read(filepath) == false) {
        FREE_SDS(filepath);
        return false;
    }

    mpack_tree_t tree;
    mpack_tree_init_filename(&tree, filepath, 0);
    mpack_tree_set_error_handler(&tree, log_mpack_node_error);
    FREE_SDS(filepath);
    mpack_tree_parse(&tree);
    mpack_node_t root = mpack_tree_root(&tree);

    // check for expected jukebox_mode
    enum jukebox_modes jukebox_mode = (enum jukebox_modes)mpack_node_int(mpack_node_map_cstr(root, "jukeboxMode"));
    if (jukebox_mode != partition_state->jukebox.mode) {
        mpack_tree_destroy(&tree);
        MYMPD_LOG_INFO(NULL, "Unexpected jukebox mode, ignoring saved jukebox queue");
        return false;
    }

    mpack_node_t entries_node = mpack_node_map_cstr(root, "entries");
    size_t len = mpack_node_array_length(entries_node);
    for (size_t i = 0; i < len; i++) {
        mpack_node_t entry = mpack_node_array_at(entries_node, i);
        mpack_node_t entry_uri = mpack_node_map_cstr(entry, "uri");
        const char *uri = mpack_node_str(entry_uri);
        size_t uri_len = mpack_node_strlen(entry_uri);
        int64_t lineno = mpack_node_i64(mpack_node_map_cstr(entry, "lineno"));
        mpack_node_t entry_tagvalue = mpack_node_map_cstr(entry, "tagvalue");
        const char *tagvalue = mpack_node_str(entry_tagvalue);
        size_t tagvalue_len = mpack_node_strlen(entry_tagvalue);
        list_push_len(partition_state->jukebox.queue, uri, uri_len, lineno, tagvalue, tagvalue_len, NULL);
    }
    // clean up and check for errors
    bool rc = mpack_tree_destroy(&tree) != mpack_ok
        ? false
        : true;
    MYMPD_LOG_INFO(NULL, "Read %u jukebox queue entries from disc", partition_state->last_played.length);
    return rc;
}
