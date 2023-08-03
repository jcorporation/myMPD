/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2023 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#ifndef MYMPD_API_SETTINGS_H
#define MYMPD_API_SETTINGS_H

#include "dist/sds/sds.h"
#include "src/lib/jsonrpc.h"
#include "src/lib/mympd_state.h"
#include "src/lib/validate.h"

bool settings_to_webserver(struct t_mympd_state *mympd_state);
void mympd_api_settings_statefiles_global_read(struct t_mympd_state *mympd_state);
void mympd_api_settings_statefiles_partition_read(struct t_partition_state *partition_state);
sds mympd_api_settings_get(struct t_partition_state *partition_state, sds buffer, long request_id);
bool mympd_api_settings_cols_save(struct t_mympd_state *mympd_state, sds table, sds cols);
bool mympd_api_settings_set(const char *path, sds key, sds value, int vtype, validate_callback vcb, void *userdata, struct t_jsonrpc_parse_error *error);
bool mympd_api_settings_mpd_options_set(const char *path, sds key, sds value, int vtype, validate_callback vcb, void *userdata, struct t_jsonrpc_parse_error *error);
bool mympd_api_settings_connection_save(const char *path, sds key, sds value, int vtype, validate_callback vcb, void *userdata, struct t_jsonrpc_parse_error *error);
bool mympd_api_settings_partition_set(const char *path, sds key, sds value, int vtype, validate_callback vcb, void *userdata, struct t_jsonrpc_parse_error *error);
#endif
