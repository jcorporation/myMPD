/*
 SPDX-License-Identifier: GPL-2.0-or-later
 myMPD (c) 2018-2021 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#ifndef __MYMPD_PIN_H__
#define __MYMPD_PIN_H__
void mympd_set_pin(sds workdir);
sds hash_pin(const char *pin);
#endif
