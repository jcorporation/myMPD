/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2024 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#ifndef MYMPD_API_SCRIPTS_VARS_H
#define MYMPD_API_SCRIPTS_VARS_H

#include "dist/sds/sds.h"
#include "src/lib/list.h"

#include <stdbool.h>

bool mympd_api_script_vars_delete(struct t_list *script_var_list, sds key);
bool mympd_api_script_vars_save(struct t_list *script_var_list, sds key, sds value);
bool mympd_api_script_vars_file_read(struct t_list *script_var_list, sds workdir);
bool mympd_api_script_vars_file_save(struct t_list *script_var_list, sds workdir);
sds mympd_api_script_vars_list(struct t_list *script_var_list, sds buffer, unsigned request_id);

#endif
