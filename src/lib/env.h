/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2025 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

/*! \file
 * \brief Environment handling
 */

#ifndef MYMPD_ENV_H
#define MYMPD_ENV_H

#include "dist/sds/sds.h"
#include "src/lib/validate.h"

#include <stdbool.h>

sds getenv_string(const char *env_var, const char *default_value, validate_callback vcb);
int getenv_int(const char *env_var, int default_value, int min, int max);
unsigned getenv_uint(const char *env_var, unsigned default_value, unsigned min, unsigned max);
bool getenv_bool(const char *env_var, bool default_value);

#endif
