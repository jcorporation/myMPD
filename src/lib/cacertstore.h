/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2025 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

/*! \file
 * \brief CA cert store handling
 */

#ifndef MYMPD_CACERTSTORE_H
#define MYMPD_CACERTSTORE_H

#include <stdbool.h>

const char *find_ca_cert_store(bool silent);

#endif
