/*
 SPDX-License-Identifier: GPL-2.0-or-later
 myMPD (c) 2018-2021 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#ifndef __CONFIG_H__
#define __CONFIG_H__
void mympd_config_defaults(struct t_config *config);
void mympd_free_config(struct t_config *config);
bool mympd_read_config(struct t_config *config);
#endif
