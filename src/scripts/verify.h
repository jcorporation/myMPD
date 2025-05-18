/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2025 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

/*! \file
 * \brief Variables API functions
 */

#ifndef MYMPD_SCRIPTS_VERIFY_H
#define MYMPD_SCRIPTS_VERIFY_H

#include "dist/sds/sds.h"

#include <stdbool.h>

bool script_sig_verify(sds script, sds signature_base64);

#endif
