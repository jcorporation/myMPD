/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2025 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

/*! \file
 * \brief Linux passwd helper functions
 */

#ifndef MYMPD_PASSWD_H
#define MYMPD_PASSWD_H

#include <pwd.h>

struct passwd *get_passwd_entry(struct passwd *pwd, const char *username);

#endif
