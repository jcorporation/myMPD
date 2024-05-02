/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2024 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include "compile_time.h"
#include "src/lib/last_played.h"

#include "src/lib/filehandler.h"
#include "src/lib/log.h"
#include "src/lib/mpack.h"
#include "src/lib/sds_extras.h"

#include <unistd.h>

/*
 * Public functions
 */

/**
 * Saves the last played list to disc
 * @param partition_state pointer to partition state
 * @return true on success, else false
 */
bool last_played_file_save(struct t_partition_state *partition_state) {
    MYMPD_LOG_INFO(partition_state->name, "Saving %u last_played entries to disc", partition_state->last_played.length);
    mpack_writer_t writer;
    sds tmp_file = sdscatfmt(sdsempty(), "%S/%S/%s.XXXXXX",
        partition_state->config->workdir, partition_state->state_dir, FILENAME_LAST_PLAYED);
    FILE *fp = open_tmp_file(tmp_file);
    if (fp == NULL) {
        FREE_SDS(tmp_file);
        return false;
    }
    // init mpack
    mpack_writer_init_stdfile(&writer, fp, true);
    mpack_writer_set_error_handler(&writer, log_mpack_write_error);

    mpack_start_array(&writer, partition_state->last_played.length);
    struct t_list_node *current = partition_state->last_played.head;
    while (current != NULL) {
        mpack_build_map(&writer);
        mpack_write_kv(&writer, "Last-Played", current->value_i);
        mpack_write_kv(&writer, "uri", current->key);
        mpack_complete_map(&writer);
        current = current->next;
    }
    mpack_finish_array(&writer);
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
 * Reads the last played list from disc
 * @param partition_state pointer to partition state
 * @return true on success, else false
 */
bool last_played_file_read(struct t_partition_state *partition_state) {
    sds filepath = sdscatfmt(sdsempty(), "%S/%s/%s",
        partition_state->config->workdir, partition_state->state_dir, FILENAME_LAST_PLAYED);
    mpack_tree_t tree;
    mpack_tree_init_filename(&tree, filepath, 0);
    mpack_tree_set_error_handler(&tree, log_mpack_node_error);
    mpack_tree_parse(&tree);
    mpack_node_t root = mpack_tree_root(&tree);
    size_t len = mpack_node_array_length(root);
    for (size_t i = 0; i < len; i++) {
        mpack_node_t entry = mpack_node_array_at(root, i);
        int64_t last_played = mpack_node_i64(mpack_node_map_cstr(entry, "Last-Played"));
        const char *uri = mpack_node_str(mpack_node_map_cstr(entry, "uri"));
        size_t uri_len = mpack_node_strlen(mpack_node_map_cstr(entry, "uri"));
        list_push_len(&partition_state->last_played, uri, uri_len, last_played, NULL, 0, NULL);
    }
    // clean up and check for errors
    bool rc = mpack_tree_destroy(&tree) != mpack_ok
        ? false
        : true;
    MYMPD_LOG_INFO(NULL, "Read %u last_played entries from disc", partition_state->last_played.length);
    FREE_SDS(filepath);
    return rc;
}
