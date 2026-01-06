/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2026 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

/*! \file
 * \brief Compatibility functions for older libmpdclient versions
 */

#ifndef MYMPD_MPD_CLIENT_COMPAT_H
#define MYMPD_MPD_CLIENT_COMPAT_H

/**
 * Fix for typo in libmpdclient < 2.25
 */
#define MPD_STICKER_OP_UNKNOWN -1
#define	MPD_STICKER_SORT_UNKNOWN -1

#endif
