/*
 SPDX-License-Identifier: GPL-2.0-or-later
 myMPD (c) 2018-2020 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#ifndef __MYMPD_API_SCRIPTS_H
#define __MYMPD_API_SCRIPTS_H
sds mympd_api_script_execute(t_config *config, sds buffer, sds method, int request_id, const char *script);
#endif
