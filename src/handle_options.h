/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2022 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#ifndef MYMPD_HANDLE_OPTIONS_H
#define MYMPD_HANDLE_OPTIONS_H

#include "lib/mympd_configuration.h"

#include <stdbool.h>

int handle_options(struct t_config *config, int argc, char **argv);

#endif
