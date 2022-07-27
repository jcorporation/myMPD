/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2022 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#ifndef MYMPD_API_TRIGGER_H
#define MYMPD_API_TRIGGER_H

#include "../lib/mympd_state.h"

sds mympd_api_trigger_list(struct t_list *trigger_list, sds buffer, long request_id);
sds mympd_api_trigger_get(struct t_list *trigger_list, sds buffer, long request_id, long id);
bool mympd_api_trigger_file_read(struct t_list *trigger_list, sds workdir);
bool mympd_api_trigger_file_save(struct t_list *trigger_list, sds workdir);
void mympd_api_trigger_execute(struct t_list *trigger_list, enum trigger_events event);
void mympd_api_trigger_execute_feedback(struct t_list *trigger_list, sds uri, int vote);
bool mympd_api_trigger_delete(struct t_list *trigger_list, long idx);
const char *mympd_api_trigger_name(long event);
sds mympd_api_trigger_print_trigger_list(sds buffer);
#endif
