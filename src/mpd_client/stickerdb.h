/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2023 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#ifndef MYMPD_MPD_CLIENT_STICKERDB_H
#define MYMPD_MPD_CLIENT_STICKERDB_H

#include "src/lib/mympd_state.h"

bool stickerdb_connect(struct t_partition_state *partition_state);
bool stickerdb_idle(struct t_partition_state *partition_state);
bool stickerdb_enter_idle(struct t_partition_state *partition_state);
bool stickerdb_exit_idle(struct t_partition_state *partition_state);

sds stickerdb_get(struct t_partition_state *partition_state, const char *uri, const char *name);
long long stickerdb_get_llong(struct t_partition_state *partition_state, const char *uri, const char *name);
struct t_sticker *stickerdb_get_all(struct t_partition_state *partition_state, const char *uri, struct t_sticker *sticker, bool user_defined);

sds stickerdb_get_batch(struct t_partition_state *partition_state, const char *uri, const char *name);
long long stickerdb_get_llong_batch(struct t_partition_state *partition_state, const char *uri, const char *name);
struct t_sticker *stickerdb_get_all_batch(struct t_partition_state *partition_state, const char *uri, struct t_sticker *sticker, bool user_defined);

rax *stickerdb_find_stickers_by_name(struct t_partition_state *partition_state, const char *name);
rax *stickerdb_find_stickers_by_name_value(struct t_partition_state *partition_state,
        const char *name, const char *op, const char *value);
void stickerdb_free_find_result(rax *stickers);

bool stickerdb_set(struct t_partition_state *partition_state, const char *uri, const char *name, const char *value);
bool stickerdb_set_llong(struct t_partition_state *partition_state, const char *uri, const char *name, long long value);
bool stickerdb_inc(struct t_partition_state *partition_state, const char *uri, const char *name);
bool stickerdb_inc_set(struct t_partition_state *partition_state, const char *uri,
        enum mympd_sticker_types name_inc, enum mympd_sticker_types name_timestamp, time_t timestamp);

bool stickerdb_set_elapsed(struct t_partition_state *partition_state, const char *uri, time_t elapsed);
bool stickerdb_inc_play_count(struct t_partition_state *partition_state, const char *uri, time_t timestamp);
bool stickerdb_inc_skip_count(struct t_partition_state *partition_state, const char *uri);
bool stickerdb_set_like(struct t_partition_state *partition_state, const char *uri, enum sticker_like value);
bool stickerdb_set_rating(struct t_partition_state *partition_state, const char *uri, int value);

#endif
