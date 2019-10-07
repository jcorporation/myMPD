/* myMPD
   (c) 2018-2019 Juergen Mang <mail@jcgames.de>
   This project's homepage is: https://github.com/jcorporation/mympd
   
   myMPD ist fork of:
   
   ympd
   (c) 2013-2014 Andrew Karpow <andy@ndyk.de>
   This project's homepage is: http://www.ympd.org
   
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; version 2 of the License.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License along
   with this program; if not, write to the Free Software Foundation, Inc.,
   Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
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
