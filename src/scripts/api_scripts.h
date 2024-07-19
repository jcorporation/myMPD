/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2024 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

/*! \file
 * \brief Scripts API functions
 */

#ifndef MYMPD_SCRIPTS_SCRIPTS_API_H
#define MYMPD_SCRIPTS_SCRIPTS_API_H

#include "dist/sds/sds.h"
#include "src/lib/list.h"
#include "src/scripts/util.h"

#include <stdbool.h>

bool scripts_file_reload(struct t_scripts_state *scripts_state);
bool scripts_file_read(struct t_scripts_state *scripts_state);
bool script_save(struct t_scripts_state *scripts_state, sds scriptname, sds oldscript,
        sds file, int order, int version, sds content, struct t_list *arguments, sds *error);
bool script_delete(struct t_scripts_state *scripts_state, sds scriptname);
sds script_get(struct t_list *script_list, sds buffer, unsigned request_id, sds scriptname);
sds script_list(struct t_list *script_list, sds buffer, unsigned request_id, bool all);

#endif
