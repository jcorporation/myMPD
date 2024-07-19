/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2024 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

/*! \file
 * \brief Certificate handling
 */

#ifndef MYMPD_CERT_H
#define MYMPD_CERT_H

#include "dist/sds/sds.h"

#include <stdbool.h>

bool certificates_check(sds workdir, sds ssl_san);
sds certificate_get_detail(sds cert_content);

#endif
