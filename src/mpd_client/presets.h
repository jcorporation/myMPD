/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2023 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#ifndef MYMPD_MPD_CLIENT_PRESETS_H
#define MYMPD_MPD_CLIENT_PRESETS_H

#include "src/lib/list.h"
#include "src/lib/mympd_state.h"

bool preset_apply(struct t_partition_state *partition_state, sds preset_name, sds *error);
sds presets_list(struct t_list *preset_list, sds buffer);
bool preset_save(struct t_list *preset_list, sds preset_name, sds preset_value, sds *error);
bool preset_delete(struct t_list *preset_list, const char *preset_name);
bool preset_list_save(struct t_partition_state *partition_state);
bool preset_list_load(struct t_partition_state *partition_state);

#endif
