/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2024 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#ifndef MYMPD_SCRIPTS_SCRIPTS_API_H
#define MYMPD_SCRIPTS_SCRIPTS_API_H

#include "dist/sds/sds.h"
#include "src/lib/list.h"

#include <stdbool.h>

bool script_save(sds workdir, sds script, sds oldscript, int order, sds content, struct t_list *arguments, sds *error);
bool script_delete(sds workdir, sds script);
sds script_get(sds workdir, sds buffer, unsigned request_id, sds script);
sds script_list(sds workdir, sds buffer, unsigned request_id, bool all);

#endif
