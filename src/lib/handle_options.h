/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2025 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

/*! \file
 * \brief Command line options handling
 */

#ifndef MYMPD_HANDLE_OPTIONS_H
#define MYMPD_HANDLE_OPTIONS_H

#include "src/lib/config_def.h"

#include <stdbool.h>

/**
 * Return codes for options handler
 */
enum handle_options_rc {
    OPTIONS_RC_INVALID = -1,
    OPTIONS_RC_OK = 0,
    OPTIONS_RC_EXIT = 1
};

enum handle_options_rc handle_options(struct t_config *config, int argc, char **argv);

#endif
