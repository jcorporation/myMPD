/*
 SPDX-License-Identifier: GPL-2.0-or-later
 myMPD (c) 2018-2021 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#ifndef _MYMPD_AUTOCONF_H
#define _MYMPD_AUTOCONF_H
sds find_mpd_conf(void);
sds get_mpd_conf(const char *key, const char *default_value);
#endif
