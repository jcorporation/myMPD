/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2022 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#ifndef MYMPD_API_SETTINGS_H
#define MYMPD_API_SETTINGS_H

#include "../../dist/sds/sds.h"
#include "../lib/mympd_state.h"
#include "../lib/validate.h"

void mympd_api_settings_statefiles_read(struct t_mympd_state *mympd_state);
sds mympd_api_settings_get(struct t_mympd_state *mympd_state, sds buffer, sds method, long request_id);
bool mympd_api_settings_cols_save(struct t_mympd_state *mympd_state, sds table, sds cols);
bool mympd_api_settings_set(sds key, sds value, int vtype, validate_callback vcb, void *userdata, sds *error);
bool mympd_api_settings_mpd_options_set(sds key, sds value, int vtype, validate_callback vcb, void *userdata, sds *error);
bool mympd_api_settings_connection_save(sds key, sds value, int vtype, validate_callback vcb, void *userdata, sds *error);
#endif
