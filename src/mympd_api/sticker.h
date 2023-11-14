/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2023 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#ifndef MYMPD_API_STICKER_H
#define MYMPD_API_STICKER_H

#include "src/lib/mympd_state.h"

bool mympd_api_sticker_set_feedback(struct t_stickerdb_state *stickerdb, struct t_list *trigger_list, const char *partition_name,
    sds uri, enum feedback_type type, int value, sds *error);
sds mympd_api_sticker_get_print(sds buffer, struct t_stickerdb_state *stickerdb, const char *uri, const struct t_tags *tags);
sds mympd_api_sticker_get_print_batch(sds buffer, struct t_stickerdb_state *stickerdb, const char *uri, const struct t_tags *tags);
sds mympd_api_sticker_print(sds buffer, struct t_sticker *sticker, const struct t_tags *tags);

#endif
