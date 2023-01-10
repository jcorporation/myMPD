/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2023 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#ifndef MYMPD_CERT_H
#define MYMPD_CERT_H

#include "dist/sds/sds.h"

#include <stdbool.h>

bool certificates_check(sds workdir, sds ssl_san);
#endif
