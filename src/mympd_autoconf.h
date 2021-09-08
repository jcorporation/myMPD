/*
 SPDX-License-Identifier: GPL-2.0-or-later
 myMPD (c) 2018-2021 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#ifndef MYMPD_AUTOCONF_H
#define MYMPD_AUTOCONF_H

#include "../dist/src/sds/sds.h"
#include "lib/validate.h"

sds find_mpd_conf(void);
sds get_mpd_conf(const char *key, const char *default_value, validate_callback vcb);
#endif
