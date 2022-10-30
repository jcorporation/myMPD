/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2022 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#ifndef MYMPD_JUKEBOX_H
#define MYMPD_JUKEBOX_H

#include "src/lib/api.h"
#include "src/lib/mympd_state.h"

enum jukebox_modes jukebox_mode_parse(const char *str);
const char *jukebox_mode_lookup(enum jukebox_modes mode);
void jukebox_clear_all(struct t_mympd_state *mympd_state);
void jukebox_clear(struct t_list *list);
bool jukebox_rm_entry(struct t_list *list, long pos);
sds jukebox_list(struct t_partition_state *partition_state, sds buffer, enum mympd_cmd_ids cmd_id,
        long request_id, long offset, long limit, sds searchstr,
        const struct t_tags *tagcols);
bool jukebox_run(struct t_partition_state *partition_state);
bool jukebox_add_to_queue(struct t_partition_state *partition_state, long add_songs,
        enum jukebox_modes jukebox_mode, const char *playlist, bool manual);
#endif
