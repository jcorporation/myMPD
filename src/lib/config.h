/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2022 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#ifndef MYMPD_CONFIG_H
#define MYMPD_CONFIG_H

#include "src/lib/config_def.h"

#include <stdbool.h>

void mympd_config_defaults_initial(struct t_config *config);
void mympd_config_defaults(struct t_config *config);
void *mympd_config_free(struct t_config *config);
bool mympd_config_rw(struct t_config *config, bool write);
void mympd_autoconf(struct t_config *config);
#endif
