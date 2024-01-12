/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2024 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include "compile_time.h"
#include "src/lib/rax_extras.h"

#include "src/lib/random.h"
#include "src/lib/sds_extras.h"

#include <stdlib.h>

/**
 * Inserts the key into the radix tree.
 * Appends random chars to the key until it is uniq.
 * @param r rax tree to insert
 * @param key key to insert
 * @param data data to insert
 */
void rax_insert_no_dup(rax *r, sds key, void *data) {
    if (raxTryInsert(r, (unsigned char*)key, sdslen(key), data, NULL) == 1) {
        return;
    }
    //duplicate - add random chars until it is uniq
    while (raxTryInsert(r, (unsigned char*)key, sdslen(key), data, NULL) == 0) {
        key = sds_catchar(key, randchar());
    }
}

/**
 * Frees a rax tree and its data with a free_cb
 * @param r rax tree to free
 * @param free_cb callback to free data or NULL to free a pointer
 */
void rax_free_data(rax *r, rax_free_data_callback free_cb) {
    raxIterator iter;
    raxStart(&iter, r);
    raxSeek(&iter, "^", NULL, 0);
    while (raxNext(&iter)) {
        if (free_cb != NULL) {
            free_cb(iter.data);
        }
        else if (iter.data != NULL) {
            free(iter.data);
        }
    }
    raxStop(&iter);
    raxFree(r);
}

/**
 * Callback to free sds data
 * @param data pointer to sds string
 */
static void rax_free_sds_data_cb(void *data) {
    FREE_SDS(data);
}

/**
 * Frees a rax tree and its sds data
 * @param r rax tree to free
 */
void rax_free_sds_data(rax *r) {
    rax_free_data(r, rax_free_sds_data_cb);
}
