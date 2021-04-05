/*
 SPDX-License-Identifier: GPL-2.0-or-later
 myMPD (c) 2018-2021 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#ifndef __STATE_FILES_H
#define __STATE_FILES_H
sds state_file_rw_string(struct t_config *config, const char *name, const char *def_value, bool warn);
bool state_file_rw_bool(struct t_config *config, const char *name, const bool def_value, bool warn);
int state_file_rw_int(struct t_config *config, const char *name, const int def_value, bool warn);
bool state_file_write(struct t_config *config, const char *name, const char *value);
#endif
