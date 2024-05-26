/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2024 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#ifndef MYMPD_SCRIPTS_LUA_H
#define MYMPD_SCRIPTS_LUA_H

#include "src/scripts/util.h"

#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>

bool script_start(struct t_scripts_state *scripts_state, sds script, struct t_list *arguments,
        const char *partition, bool localscript, enum script_start_events start_event,
        unsigned request_id, unsigned long conn_id, sds *error);
bool script_validate(struct t_config *config, sds name, sds content, sds *error);

#endif
