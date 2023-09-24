/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2023 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include "compile_time.h"
#include "src/mympd_api/sticker.h"

#include "src/lib/jsonrpc.h"
#include "src/mpd_client/stickerdb.h"
#include "src/mympd_api/trigger.h"

/**
 * Sets the like sticker
 * @param partition_state pointer to partition state
 * @param uri uri to like
 * @param like like value to set
 * @param error already allocated sds string to append the error message
 * @return true on success, else false
 */
bool mympd_api_sticker_set_like(struct t_partition_state *partition_state, sds uri, int like, sds *error) {
    if (partition_state->mpd_state->feat_stickers == false) {
        *error = sdscat(*error, "MPD stickers are disabled");
        return false;
    }
    bool rc = stickerdb_set_like(partition_state->mympd_state->stickerdb, uri, (enum sticker_like)like);
    if (rc == false) {
        *error = sdscat(*error, "Failed to set like");
        return false;
    }
    //mympd_feedback trigger
    mympd_api_trigger_execute_feedback(&partition_state->mympd_state->trigger_list, uri, like, partition_state->name);
    return true;
}

/**
 * Gets the stickers from sticker cache and returns a json list
 * Shortcut for stickerdb_get_all and print_sticker
 * @param buffer already allocated sds string to append the list
 * @param partition_state pointer to partition state
 * @param uri song uri
 * @param tags array of stickers to print
 * @return pointer to the modified buffer
 */
sds mympd_api_sticker_get_print(sds buffer, struct t_partition_state *partition_state, const char *uri, const struct t_tags *tags) {
    if (tags->stickers_len == 0) {
        return buffer;
    }
    struct t_sticker sticker;
    stickerdb_get_all(partition_state->mympd_state->stickerdb, uri, &sticker, false);
    buffer = mympd_api_sticker_print(buffer, &sticker, tags);
    sticker_struct_clear(&sticker);
    return buffer;
}

/**
 * Gets the stickers from sticker cache and returns a json list.
 * Shortcut for stickerdb_get_all_batch and print_sticker.
 * You must exit the stickerdb idle mode before.
 * @param buffer already allocated sds string to append the list
 * @param partition_state pointer to partition state
 * @param uri song uri
 * @param tags array of stickers to print
 * @return pointer to the modified buffer
 */
sds mympd_api_sticker_get_print_batch(sds buffer, struct t_partition_state *partition_state, const char *uri, const struct t_tags *tags) {
    if (tags->stickers_len == 0) {
        return buffer;
    }
    struct t_sticker sticker;
    stickerdb_get_all_batch(partition_state->mympd_state->stickerdb, uri, &sticker, false);
    buffer = mympd_api_sticker_print(buffer, &sticker, tags);
    sticker_struct_clear(&sticker);
    return buffer;
}

/**
 * Print the sticker struct as json list
 * @param buffer already allocated sds string to append the list
 * @param sticker pointer to sticker struct to print
 * @param tags array of stickers to print
 * @return pointer to the modified buffer
 */
sds mympd_api_sticker_print(sds buffer, struct t_sticker *sticker, const struct t_tags *tags) {
    if (sticker == NULL) {
        return buffer;
    }
    for (size_t i = 0; i < tags->stickers_len; i++) {
        if (i > 0) {
            buffer = sdscatlen(buffer, ",", 1);
        }
        buffer = tojson_llong(buffer, sticker_name_lookup(tags->stickers[i]), sticker->mympd[tags->stickers[i]], false);
    }
    return buffer;
}
