/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2023 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include "compile_time.h"
#include "src/mpd_client/stickerdb.h"
#include "src/mympd_api/sticker.h"

#include "src/lib/jsonrpc.h"
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
 * Shortcut for get_sticker_from_cache and print_sticker
 * @param buffer already allocated sds string to append the list
 * @param partition_state pointer to partition state
 * @param uri song uri
 * @return pointer to the modified buffer
 */
sds mympd_api_sticker_get_print(sds buffer, struct t_partition_state *partition_state, const char *uri) {
    struct t_sticker sticker;
    stickerdb_get_all(partition_state->mympd_state->stickerdb, uri, &sticker, false);
    buffer = mympd_api_sticker_print(buffer, &sticker);
    sticker_struct_clear(&sticker);
    return buffer;
}

/**
 * Print the sticker struct as json list
 * @param buffer already allocated sds string to append the list
 * @param sticker pointer to sticker struct to print
 * @return pointer to the modified buffer
 */
sds mympd_api_sticker_print(sds buffer, struct t_sticker *sticker) {
    if (sticker != NULL) {
        buffer = tojson_long(buffer, "stickerPlayCount", sticker->play_count, true);
        buffer = tojson_long(buffer, "stickerSkipCount", sticker->skip_count, true);
        buffer = tojson_long(buffer, "stickerLike", sticker->like, true);
        buffer = tojson_time(buffer, "stickerLastPlayed", sticker->last_played, true);
        buffer = tojson_time(buffer, "stickerLastSkipped", sticker->last_skipped, true);
        buffer = tojson_time(buffer, "stickerElapsed", sticker->elapsed, false);
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
