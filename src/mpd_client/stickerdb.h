/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2024 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#ifndef MYMPD_MPD_CLIENT_STICKERDB_H
#define MYMPD_MPD_CLIENT_STICKERDB_H

#include "src/lib/mympd_state.h"

bool stickerdb_connect(struct t_stickerdb_state *stickerdb);
void stickerdb_disconnect(struct t_stickerdb_state *stickerdb, enum mpd_conn_states new_conn_state);
bool stickerdb_idle(struct t_stickerdb_state *stickerdb);
bool stickerdb_enter_idle(struct t_stickerdb_state *stickerdb);
bool stickerdb_exit_idle(struct t_stickerdb_state *stickerdb);
bool stickerdb_check_error_and_recover(struct t_stickerdb_state *stickerdb, const char *command);

sds stickerdb_get(struct t_stickerdb_state *stickerdb, const char *uri, const char *name);
int64_t stickerdb_get_int64(struct t_stickerdb_state *stickerdb, const char *uri, const char *name);
struct t_sticker *stickerdb_get_all(struct t_stickerdb_state *stickerdb, const char *uri, struct t_sticker *sticker, bool user_defined);

sds stickerdb_get_batch(struct t_stickerdb_state *stickerdb, const char *uri, const char *name);
int64_t stickerdb_get_int64_batch(struct t_stickerdb_state *stickerdb, const char *uri, const char *name);
struct t_sticker *stickerdb_get_all_batch(struct t_stickerdb_state *stickerdb, const char *uri, struct t_sticker *sticker, bool user_defined);

rax *stickerdb_find_stickers_by_name(struct t_stickerdb_state *stickerdb, const char *name);
rax *stickerdb_find_stickers_by_name_value(struct t_stickerdb_state *stickerdb,
        const char *name, enum mpd_sticker_operator op, const char *value);
void stickerdb_free_find_result(rax *stickers);

struct t_list *stickerdb_find_stickers_sorted(struct t_stickerdb_state *stickerdb,
        const char *name, enum mpd_sticker_operator op, const char *value,
        enum mpd_sticker_sort sort, bool sort_desc, unsigned start, unsigned end);

bool stickerdb_set(struct t_stickerdb_state *stickerdb, const char *uri, const char *name, const char *value);
bool stickerdb_set_int64(struct t_stickerdb_state *stickerdb, const char *uri, const char *name, int64_t value);
bool stickerdb_inc(struct t_stickerdb_state *stickerdb, const char *uri, const char *name);
bool stickerdb_inc_set(struct t_stickerdb_state *stickerdb, const char *uri,
        enum mympd_sticker_types name_inc, enum mympd_sticker_types name_timestamp, time_t timestamp);

bool stickerdb_set_elapsed(struct t_stickerdb_state *stickerdb, const char *uri, time_t elapsed);
bool stickerdb_inc_play_count(struct t_stickerdb_state *stickerdb, const char *uri, time_t timestamp);
bool stickerdb_inc_skip_count(struct t_stickerdb_state *stickerdb, const char *uri);
bool stickerdb_set_like(struct t_stickerdb_state *stickerdb, const char *uri, enum sticker_like value);
bool stickerdb_set_rating(struct t_stickerdb_state *stickerdb, const char *uri, int value);
bool stickerdb_remove(struct t_stickerdb_state *stickerdb, const char *uri, const char *name);

#endif
