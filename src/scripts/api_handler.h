/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2025 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

/*! \file
 * \brief Scripts thread API handler
 */

#ifndef MYMPD_SCRIPTS_API_HANDLER_H
#define MYMPD_SCRIPTS_API_HANDLER_H

#include "src/scripts/util.h"
#include "src/lib/api.h"

void scripts_api_handler(struct t_scripts_state *scripts_state, struct t_work_request *request);

#endif
