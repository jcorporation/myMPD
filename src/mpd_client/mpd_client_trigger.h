/*
 SPDX-License-Identifier: GPL-2.0-or-later
 myMPD (c) 2018-2021 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#ifndef MYMPD_MPD_CLIENT_TRIGGER_H
#define MYMPD_MPD_CLIENT_TRIGGER_H

#include "../mympd_state.h"

sds trigger_list(struct t_mympd_state *mympd_state, sds buffer, sds method, long request_id);
sds trigger_get(struct t_mympd_state *mympd_state, sds buffer, sds method, long request_id, int id);
bool triggerfile_read(struct t_mympd_state *mympd_state);
bool triggerfile_save(struct t_mympd_state *mympd_state);
void free_trigerlist_arguments(struct t_mympd_state *mympd_state);
void trigger_execute(struct t_mympd_state *mympd_state, enum trigger_events event);
bool delete_trigger(struct t_mympd_state *mympd_state, unsigned idx);
const char *trigger_name(long event);
sds print_trigger_list(sds buffer);
#endif
