/*
 SPDX-License-Identifier: GPL-2.0-or-later
 myMPD (c) 2018-2021 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#ifndef MYMPD_API_SCRIPTS_H
#define MYMPD_API_SCRIPTS_H

#include "../../dist/src/sds/sds.h"
#include "../lib/list.h"
#include <stdbool.h>

#ifdef ENABLE_LUA
    #define LUA_VERSION_5_3(major, minor) \
            ((5) == LUA_VERSION_MAJOR && \
            ((3) == LUA_VERSION_MINOR))

    bool mympd_api_script_save(struct t_config *config, const char *script, int order, const char *content, const char *arguments, const char *oldscript);
    bool mympd_api_script_delete(struct t_config *config, const char *script);
    sds mympd_api_script_get(struct t_config *config, sds buffer, sds method, long request_id, const char *script);
    sds mympd_api_script_list(struct t_config *config, sds buffer, sds method, long request_id, bool all);
    bool mympd_api_script_start(struct t_config *config, const char *script, struct list *arguments, bool localscript);
#endif
#endif
