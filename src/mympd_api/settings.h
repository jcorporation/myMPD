/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2025 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

/*! \file
 * \brief myMPD settings API
 */

#ifndef MYMPD_API_SETTINGS_H
#define MYMPD_API_SETTINGS_H

#include "dist/sds/sds.h"
#include "src/lib/json/json_query.h"
#include "src/lib/mympd_state.h"
#include "src/lib/validate.h"

bool settings_to_webserver(struct t_mympd_state *mympd_state);
void mympd_api_settings_statefiles_global_read(struct t_mympd_state *mympd_state);
void mympd_api_settings_statefiles_partition_read(struct t_partition_state *partition_state);
sds mympd_api_settings_get(struct t_mympd_state *mympd_state, struct t_partition_state *partition_state, sds buffer, unsigned request_id);
bool mympd_api_settings_view_save(struct t_mympd_state *mympd_state, sds view, sds mode, sds cols);
bool mympd_api_settings_set(const char *path, sds key, sds value, enum json_vtype vtype, validate_callback vcb, void *userdata, struct t_json_parse_error *error);
bool mympd_api_settings_mpd_options_set(const char *path, sds key, sds value, enum json_vtype vtype, validate_callback vcb, void *userdata, struct t_json_parse_error *error);
bool mympd_api_settings_connection_save(const char *path, sds key, sds value, enum json_vtype vtype, validate_callback vcb, void *userdata, struct t_json_parse_error *error);
bool mympd_api_settings_partition_set(const char *path, sds key, sds value, enum json_vtype vtype, validate_callback vcb, void *userdata, struct t_json_parse_error *error);
#endif
