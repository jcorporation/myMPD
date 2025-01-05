/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2025 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

/*! \file
 * \brief Lua script functions
 */

#ifndef MYMPD_SCRIPTS_LUA_H
#define MYMPD_SCRIPTS_LUA_H

#include "src/scripts/util.h"

#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>

bool script_start(struct t_scripts_state *scripts_state, sds scriptname, struct t_list *arguments,
        const char *partition, bool localscript, enum script_start_events start_event,
        unsigned request_id, unsigned long conn_id, sds *error);
bool script_validate(struct t_config *config, sds scriptname, sds script, sds *error);

#endif
