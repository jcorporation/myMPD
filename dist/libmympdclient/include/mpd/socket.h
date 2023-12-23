// SPDX-License-Identifier: BSD-2-Clause
// Copyright The Music Player Daemon Project

/*! \file
 * \brief MPD client library
 *
 * Do not include this header directly.  Use mpd/client.h instead.
 */

#ifndef LIBMPDCLIENT_SOCKET_H
#define LIBMPDCLIENT_SOCKET_H

#ifdef WIN32
#include <winsock2.h>
typedef SOCKET mpd_socket_t;
#define MPD_INVALID_SOCKET INVALID_SOCKET
#else
typedef int mpd_socket_t;
#define MPD_INVALID_SOCKET -1
#endif

#endif
