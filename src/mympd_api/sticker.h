/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2024 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

/*! \file
 * \brief myMPD sticker API
 */

#ifndef MYMPD_API_STICKER_H
#define MYMPD_API_STICKER_H

#include "src/lib/mympd_state.h"

sds mympd_api_sticker_get(struct t_stickerdb_state *stickerdb, sds buffer, unsigned request_id,
        sds uri, enum mympd_sticker_type type, sds name);
sds mympd_api_sticker_list(struct t_stickerdb_state *stickerdb, sds buffer, unsigned request_id,
        sds uri, enum mympd_sticker_type type);
bool mympd_api_sticker_set_feedback(struct t_stickerdb_state *stickerdb, struct t_list *trigger_list, const char *partition_name,
        enum mympd_sticker_type sticker_type, sds uri, enum mympd_feedback_type feedback_type, int value, sds *error);
sds mympd_api_sticker_get_print(sds buffer, struct t_stickerdb_state *stickerdb,
        enum mympd_sticker_type type, const char *uri, const struct t_stickers *stickers);
sds mympd_api_sticker_get_print_batch(sds buffer, struct t_stickerdb_state *stickerdb,
        enum mympd_sticker_type type, const char *uri, const struct t_stickers *stickers);
sds mympd_api_sticker_print(sds buffer, struct t_sticker *sticker, const struct t_stickers *stickers);

sds mympd_api_sticker_print_types(sds buffer);
sds mympd_api_sticker_names(struct t_stickerdb_state *stickerdb, sds buffer, unsigned request_id,
        sds searchstr, enum mympd_sticker_type type);
sds mympd_api_get_sticker_uri(struct t_mympd_state *mympd_state, sds uri, enum mympd_sticker_type *type);
enum mympd_sticker_type mympd_api_get_mpd_sticker_type(enum mympd_sticker_type type);

#endif
