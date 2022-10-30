/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2022 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#ifndef MYMPD_PIN_H
#define MYMPD_PIN_H

#include "dist/sds/sds.h"

#include <stdbool.h>

bool pin_set(sds workdir);
bool pin_validate(const char *pin, const char *pin_hash);
#endif
