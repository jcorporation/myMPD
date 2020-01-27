/*
 SPDX-License-Identifier: GPL-2.0-or-later
 myMPD (c) 2018-2020 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/
   
#ifndef __HANDLE_OPTIONS_H__
#define __HANDLE_OPTIONS_H__
bool smartpls_default(t_config *config);
bool handle_option(t_config *config, char *cmd, sds option);
#endif
