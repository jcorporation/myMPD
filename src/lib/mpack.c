/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2024 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include "compile_time.h"
#include "src/lib/mpack.h"

#include "src/lib/log.h"

void log_mpack_node_error(mpack_tree_t *tree, mpack_error_t error) {
    (void) tree;
    MYMPD_LOG_ERROR("default", "mpack error: %s", mpack_error_to_string(error));
}

void log_mpack_write_error(mpack_writer_t *writer, mpack_error_t error) {
    (void) writer;
    MYMPD_LOG_ERROR("default", "mpack error: %s", mpack_error_to_string(error));
}

/**
 * Creates a sds string from a mpack str
 * @param node node to get the key from
 * @param key key to get
 * @return newly allocated sds string
 */
sds mpackstr_sds(mpack_node_t node, const char *key) {
    mpack_node_t data = mpack_node_map_cstr(node, key);
    return sdsnewlen(mpack_node_str(data), mpack_node_data_len(data));
}

/**
 * Appends a mpack str to a sds string
 * @param node node to get the key from
 * @param key key to get
 * @return newly allocated sds string
 */
sds mpackstr_sdscat(sds s, mpack_node_t node, const char *key) {
    mpack_node_t data = mpack_node_map_cstr(node, key);
    return sdscatlen(s, mpack_node_str(data), mpack_node_data_len(data));
}
