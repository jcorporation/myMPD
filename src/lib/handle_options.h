/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2022 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#ifndef MYMPD_HANDLE_OPTIONS_H
#define MYMPD_HANDLE_OPTIONS_H

#include "mympd_state.h"

#include <stdbool.h>

enum handle_options_rc {
    OPTIONS_RC_INVALID = -1,
    OPTIONS_RC_OK = 0,
    OPTIONS_RC_EXIT = 1
};

int handle_options(struct t_config *config, int argc, char **argv);

#endif
