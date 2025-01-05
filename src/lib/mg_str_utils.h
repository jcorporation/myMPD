/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2025 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

/*! \file
 * \brief Functions for mongoose strings
 */

#ifndef MYMPD_MG_STR_UTILS_H
#define MYMPD_MG_STR_UTILS_H

#include "dist/mongoose/mongoose.h"

int mg_str_to_int(const struct mg_str *str);
unsigned mg_str_to_uint(const struct mg_str *str);

#endif
