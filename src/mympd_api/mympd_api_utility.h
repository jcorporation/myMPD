/*
 SPDX-License-Identifier: GPL-2.0-or-later
 myMPD (c) 2018-2021 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#ifndef MYMPD_API_UTILITY_H
#define MYMPD_API_UTILITY_H

#include "../lib/mympd_state.h"

sds json_to_cols(sds cols, char *str, size_t len, bool *error);
void default_mympd_state(struct t_mympd_state *mympd_state);
void free_mympd_state(struct t_mympd_state *mympd_state);
void free_mympd_state_sds(struct t_mympd_state *mympd_state);
#endif
