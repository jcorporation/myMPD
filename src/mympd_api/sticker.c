/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2024 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include "compile_time.h"
#include "src/mympd_api/sticker.h"

#include "src/lib/jsonrpc.h"
#include "src/mpd_client/stickerdb.h"
#include "src/mympd_api/trigger.h"

/**
 * Sets the like sticker and triggers the feedback event
 * @param stickerdb pointer to stickerdb
 * @param trigger_list pointer to trigger list
 * @param partition_name the partition name
 * @param uri uri to set the feedback
 * @param type feedback type
 * @param value feedback value to set
 * @param error already allocated sds string to append the error message
 * @return true on success, else false
 */
bool mympd_api_sticker_set_feedback(struct t_stickerdb_state *stickerdb, struct t_list *trigger_list, const char *partition_name,
    sds uri, enum feedback_type type, int value, sds *error)
{
    if (stickerdb->mpd_state->feat.stickers == false) {
        *error = sdscat(*error, "MPD stickers are disabled");
        return false;
    }
    bool rc = type == FEEDBACK_LIKE
        ? value == 1
            ? stickerdb_remove(stickerdb, uri, "like")
            : stickerdb_set_like(stickerdb, uri, (enum sticker_like)value)
        : value == 0
            ? stickerdb_remove(stickerdb, uri, "rating")
            : stickerdb_set_rating(stickerdb, uri, value);
    if (rc == false) {
        *error = sdscat(*error, "Failed to set feedback");
        return false;
    }
    //mympd_feedback trigger
    mympd_api_trigger_execute_feedback(trigger_list, uri, type, value, partition_name);
    return true;
}

/**
 * Gets the stickers from sticker cache and returns a json list
 * Shortcut for stickerdb_get_all and print_sticker
 * @param buffer already allocated sds string to append the list
 * @param stickerdb pointer to stickerdb
 * @param uri song uri
 * @param tags array of stickers to print
 * @return pointer to the modified buffer
 */
sds mympd_api_sticker_get_print(sds buffer, struct t_stickerdb_state *stickerdb, const char *uri, const struct t_tags *tags) {
    if (tags->stickers_len == 0) {
        return buffer;
    }
    struct t_sticker sticker;
    if (stickerdb_get_all(stickerdb, uri, &sticker, false) != NULL) {
        buffer = mympd_api_sticker_print(buffer, &sticker, tags);
        sticker_struct_clear(&sticker);
    }
    return buffer;
}

/**
 * Gets the stickers from sticker cache and returns a json list.
 * Shortcut for stickerdb_get_all_batch and print_sticker.
 * You must exit the stickerdb idle mode before.
 * @param buffer already allocated sds string to append the list
 * @param stickerdb pointer to stickerdb
 * @param uri song uri
 * @param tags array of stickers to print
 * @return pointer to the modified buffer
 */
sds mympd_api_sticker_get_print_batch(sds buffer, struct t_stickerdb_state *stickerdb, const char *uri, const struct t_tags *tags) {
    if (tags->stickers_len == 0) {
        return buffer;
    }
    struct t_sticker sticker;
    if (stickerdb_get_all_batch(stickerdb, uri, &sticker, false) != NULL) {
        buffer = mympd_api_sticker_print(buffer, &sticker, tags);
        sticker_struct_clear(&sticker);
    }
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
    buffer = json_comma(buffer);
    for (size_t i = 0; i < tags->stickers_len; i++) {
        if (i > 0) {
            buffer = sdscatlen(buffer, ",", 1);
        }
        buffer = tojson_int64(buffer, sticker_name_lookup(tags->stickers[i]), sticker->mympd[tags->stickers[i]], false);
    }
    return buffer;
}
