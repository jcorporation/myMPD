/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2022 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#ifndef MYMPD_API_SCRIPTS_H
#define MYMPD_API_SCRIPTS_H

#include "dist/sds/sds.h"
#include "src/lib/list.h"

#include <stdbool.h>

#ifdef MYMPD_ENABLE_LUA
    bool mympd_api_script_save(sds workdir, sds script, sds oldscript, int order, sds content, struct t_list *arguments);
    bool mympd_api_script_delete(sds workdir, sds script);
    sds mympd_api_script_get(sds workdir, sds buffer, long request_id, sds script);
    sds mympd_api_script_list(sds workdir, sds buffer, long request_id, bool all);
    bool mympd_api_script_start(sds workdir, sds script, sds lualibs, struct t_list *arguments,
            const char *partition, bool localscript);
#endif
#endif
