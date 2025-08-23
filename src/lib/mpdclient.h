/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2025 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

/*! \file
 * \brief Custom libmpdclient include
 */

#ifndef MYMPD_MPDCLIENT_H
#define MYMPD_MPDCLIENT_H

#ifdef MYMPD_EMBEDDED_LIBMPDCLIENT
    #include "dist/libmympdclient/include/mpd/client.h"
#else
    #include <mpd/client.h>
#endif

#endif
