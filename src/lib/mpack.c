/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2023 Juergen Mang <mail@jcgames.de>
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
