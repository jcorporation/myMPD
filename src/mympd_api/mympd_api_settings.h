/*
 SPDX-License-Identifier: GPL-2.0-or-later
 myMPD (c) 2018-2021 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#ifndef MYMPD_API_SETTINGS_H
#define MYMPD_API_SETTINGS_H

#include "../../dist/src/sds/sds.h"
#include "../lib/mympd_state.h"
#include "../lib/validate.h"

void mympd_api_read_statefiles(struct t_mympd_state *mympd_state);
sds mympd_api_settings_put(struct t_mympd_state *mympd_state, sds buffer, sds method, long request_id);
bool mympd_api_cols_save(struct t_mympd_state *mympd_state, sds table, sds cols);
void mympd_api_settings_delete(struct t_config *config);
sds mympd_api_picture_list(struct t_mympd_state *mympd_state, sds buffer, sds method, long request_id);
bool mympd_api_settings_set(sds key, sds value, int vtype, validate_callback vcb, void *userdata, sds *error);
bool mpdclient_api_options_set(sds key, sds value, int vtype, validate_callback vcb, void *userdata, sds *error);
bool mympd_api_connection_save(sds key, sds value, int vtype, validate_callback vcb, void *userdata, sds *error);
#endif
