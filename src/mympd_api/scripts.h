/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2024 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#ifndef MYMPD_API_SCRIPTS_SCRIPTS_H
#define MYMPD_API_SCRIPTS_SCRIPTS_H

#include "dist/sds/sds.h"
#include "src/lib/list.h"
#include "src/scripts/scripts.h"

#include <lua.h>
#include <stdbool.h>

bool mympd_api_script_save(sds workdir, sds script, sds oldscript, int order, sds content, struct t_list *arguments, sds *error);
bool mympd_api_script_validate(sds name, sds content, sds lualibs, sds *error);
bool mympd_api_script_delete(sds workdir, sds script);
sds mympd_api_script_get(sds workdir, sds buffer, unsigned request_id, sds script);
sds mympd_api_script_list(sds workdir, sds buffer, unsigned request_id, bool all);
bool mympd_api_script_start(sds workdir, sds script, sds lualibs, struct t_list *arguments,
        const char *partition, bool localscript, enum script_start_events start_event);

#endif
