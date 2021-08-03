/*
 SPDX-License-Identifier: GPL-2.0-or-later
 myMPD (c) 2018-2021 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/
   
#ifndef __HANDLE_OPTIONS_H__
#define __HANDLE_OPTIONS_H__

#include <stdbool.h>

#include "mympd_config_defs.h"

bool handle_options(struct t_config *config, int argc, char **argv);

#endif
