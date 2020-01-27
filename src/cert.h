/*
 SPDX-License-Identifier: GPL-2.0-or-later
 myMPD (c) 2018-2020 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#ifndef __MYMPD_CERT_H__
#define __MYMPD_CERT_H__
bool create_certificates(sds dir, sds custom_san);
bool cleanup_certificates(sds dir, const char *name);
#endif
