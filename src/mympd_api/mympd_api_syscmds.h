/*
 SPDX-License-Identifier: GPL-2.0-or-later
 myMPD (c) 2018-2020 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#ifndef __MYMPD_API_SYSCMDS_H
#define __MYMPD_API_SYSCMDS_H
sds mympd_api_syscmd(t_config *config, sds buffer, sds method, int request_id, 
                     const char *cmd);
#endif
