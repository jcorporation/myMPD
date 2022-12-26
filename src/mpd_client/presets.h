/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2022 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#ifndef MYMPD_MPD_CLIENT_PRESETS_H
#define MYMPD_MPD_CLIENT_PRESETS_H

#include "src/lib/list.h"
#include "src/lib/mympd_state.h"

sds presets_list(struct t_list *presets, sds buffer);
bool presets_delete(struct t_list *presets, const char *preset);
bool presets_save(struct t_partition_state *partition_state);
bool presets_load(struct t_partition_state *partition_state);

#endif
