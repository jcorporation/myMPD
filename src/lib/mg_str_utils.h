/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2024 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#ifndef MYMPD_MG_STR_UTILS_H
#define MYMPD_MG_STR_UTILS_H

#include "dist/mongoose/mongoose.h"

int mg_str_to_int(struct mg_str *str);
unsigned mg_str_to_uint(struct mg_str *str);

#endif
