/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2025 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

/*! \file
 * \brief Variables API functions
 */

#ifndef MYMPD_SCRIPTS_TMP_H
#define MYMPD_SCRIPTS_TMP_H

#include "dist/rax/rax.h"
#include "dist/sds/sds.h"

#include "src/scripts/util.h"

#include <stdbool.h>

/**
 * Data for tmp variables
 */
struct t_tmpvar_data {
    sds value;        //!< Variable value
    int64_t expires;  //!< Expiration as unix timestamp, 0 = delete after first access, -1 = indefinite
};

void scripts_tmp_delete(rax *scripts_tmp_list, sds key);
sds scripts_tmp_get(rax *scripts_tmp_list, sds buffer, unsigned request_id, sds key);
sds scripts_tmp_list(rax *scripts_tmp_list, sds buffer, unsigned request_id);
bool scripts_tmp_set(rax *scripts_tmp_list, sds key, sds value, int lifetime);
void script_tmp_list_should_expire(struct t_scripts_state *scripts_state);
void scripts_tmp_list_expire(rax *scripts_tmp_list, bool cleanup);

#endif
