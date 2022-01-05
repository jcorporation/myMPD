/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2022 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#ifndef MYMPD_API_HANDLER_H
#define MYMPD_API_HANDLER_H

#include "../lib/api.h"
#include "../lib/mympd_state.h"

void mympd_api_handler(struct t_mympd_state *mympd_state, struct t_work_request *request);
#endif
