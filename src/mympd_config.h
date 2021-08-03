/*
 SPDX-License-Identifier: GPL-2.0-or-later
 myMPD (c) 2018-2021 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#ifndef __MYMPD_CONFIG_H__
#define __MYMPD_CONFIG_H__

#include <stdbool.h>

#include "mympd_config_defs.h"

void mympd_config_defaults_initial(struct t_config *config);
void mympd_config_defaults(struct t_config *config);
void mympd_free_config_initial(struct t_config *config);
void mympd_free_config(struct t_config *config);
bool mympd_read_config(struct t_config *config);
void mympd_autoconf(struct t_config *config);
#endif
