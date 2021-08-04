/*
 SPDX-License-Identifier: GPL-2.0-or-later
 myMPD (c) 2018-2021 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#ifndef __MYMPD_PIN_H__
#define __MYMPD_PIN_H__

#include <stdbool.h>

#include "../dist/src/sds/sds.h"

void set_pin(sds workdir);
bool validate_pin(const char *pin, const char *pin_hash);
#endif
