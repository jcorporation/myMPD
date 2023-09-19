/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2023 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#ifndef MYMPD_MPD_CLIENT_STICKERDB_H
#define MYMPD_MPD_CLIENT_STICKERDB_H

#include "src/lib/mympd_state.h"

/**
 * myMPD sticker types
 */
enum mympd_sticker_types {
    STICKER_UNKNOWN = -1,
    STICKER_PLAY_COUNT,
    STICKER_SKIP_COUNT,
    STICKER_LIKE,
    STICKER_LAST_PLAYED,
    STICKER_LAST_SKIPPED,
    STICKER_ELAPSED,
    STICKER_COUNT
};

/**
 * Valid values for like sticker
 */
enum sticker_like {
    STICKER_LIKE_HATE = 0,
    STICKER_LIKE_NEUTRAL = 1,
    STICKER_LIKE_LOVE = 2
};

/**
 * MPD sticker values
 */
struct t_sticker {
    long play_count;        //!< number how often the song was played
    long skip_count;        //!< number how often the song was skipped
    time_t last_played;     //!< timestamp when the song was played the last time
    time_t last_skipped;    //!< timestamp when the song was skipped the last time
    time_t elapsed;         //!< recent song position
    long like;              //!< hate/neutral/love value
    struct t_list stickers; //!< list of all other stickers
};

const char *stickerdb_name_lookup(enum mympd_sticker_types sticker);
enum mympd_sticker_types stickerdb_name_parse(const char *name);

void sticker_struct_init(struct t_sticker *sticker);
void sticker_struct_clear(struct t_sticker *sticker);

bool stickerdb_connect(struct t_partition_state *partition_state);
sds stickerdb_get(struct t_partition_state *partition_state, const char *uri, const char *name, bool idle);
long long stickerdb_get_llong(struct t_partition_state *partition_state, const char *uri, const char *name, bool idle);
bool stickerdb_get_all(struct t_partition_state *partition_state, const char *uri, struct t_sticker *sticker, bool user_defined);
bool stickerdb_set(struct t_partition_state *partition_state, const char *uri, const char *name, const char *value, bool idle);
bool stickerdb_set_llong(struct t_partition_state *partition_state, const char *uri, const char *name, long long value, bool idle);
bool stickerdb_inc(struct t_partition_state *partition_state, const char *uri, const char *name, bool idle);

bool stickerdb_set_elapsed(struct t_partition_state *partition_state, const char *uri, time_t elapsed);
bool stickerdb_inc_play_count(struct t_partition_state *partition_state, const char *uri, time_t timestamp);
bool stickerdb_inc_skip_count(struct t_partition_state *partition_state, const char *uri);
bool stickerdb_set_like(struct t_partition_state *partition_state, const char *uri, enum sticker_like value);

time_t stickerdb_get_last_played(struct t_partition_state *partition_state, const char *uri);

#endif
