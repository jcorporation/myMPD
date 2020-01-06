/*
 SPDX-License-Identifier: GPL-2.0-or-later
 myMPD (c) 2018-2020 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#ifndef __MYMPD_API_SETTINGS_H
#define __MYMPD_API_SETTINGS_H
void mympd_api_read_statefiles(t_config *config, t_mympd_state *mympd_state);
sds state_file_rw_string(t_config *config, const char *name, const char *def_value, bool warn);
bool state_file_rw_bool(t_config *config, const char *name, const bool def_value, bool warn);
int state_file_rw_int(t_config *config, const char *name, const int def_value, bool warn);
bool state_file_write(t_config *config, const char *name, const char *value);
sds mympd_api_settings_put(t_config *config, t_mympd_state *mympd_state, sds buffer, sds method, int request_id);
void mympd_api_settings_reset(t_config *config, t_mympd_state *mympd_state);
bool mympd_api_cols_save(t_config *config, t_mympd_state *mympd_state, const char *table, const char *cols);
bool mympd_api_settings_set(t_config *config, t_mympd_state *mympd_state, struct json_token *key, struct json_token *val);
bool mympd_api_connection_save(t_config *config, t_mympd_state *mympd_state, struct json_token *key, struct json_token *val);
void mympd_api_settings_delete(t_config *config);
#endif
