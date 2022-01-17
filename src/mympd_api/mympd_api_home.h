/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2022 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#ifndef MYMPD_API_HOME_H
#define MYMPD_API_HOME_H

#include "../../dist/sds/sds.h"
#include "../lib/mympd_state.h"

#include <stdbool.h>

bool mympd_api_home_icon_move(struct t_mympd_state *mympd_state, long from, long to);
bool mympd_api_home_icon_delete(struct t_mympd_state *mympd_state, long pos);
bool mympd_api_home_icon_save(struct t_mympd_state *mympd_state, bool replace, long oldpos,
    const char *name, const char *ligature, const char *bgcolor, const char *color, const char *image,
    const char *cmd, struct t_list *option_list);
sds mympd_api_home_icon_list(struct t_mympd_state *mympd_state, sds buffer, sds method,
    long request_id);
sds mympd_api_home_icon_get(struct t_mympd_state *mympd_state, sds buffer, sds method,
    long request_id, long pos);
bool mympd_api_home_file_read(struct t_mympd_state *mympd_state);
bool mympd_api_home_file_save(struct t_mympd_state *mympd_state);
#endif
