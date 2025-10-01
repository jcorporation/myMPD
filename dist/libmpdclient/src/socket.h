// SPDX-License-Identifier: BSD-2-Clause
// Copyright The Music Player Daemon Project

#ifndef MPD_SOCKET_H
#define MPD_SOCKET_H

#include <mpd/socket.h>

#include <stdbool.h>

#ifdef _WIN32
#  include <winsock2.h>
#else
#  include <errno.h>
#endif

struct timeval;
struct mpd_error_info;

#ifdef _WIN32
bool
mpd_socket_global_init(struct mpd_error_info *error);
#else
static inline bool
mpd_socket_global_init(struct mpd_error_info *error)
{
	(void)error;
	return true;
}
#endif

static inline int
mpd_socket_errno(void)
{
#ifdef _WIN32
	return WSAGetLastError();
#else
	return errno;
#endif
}

/**
 * Can this error code be ignored?
 */
static inline bool
mpd_socket_ignore_errno(int e)
{
#ifdef _WIN32
	return e == WSAEINTR || e == WSAEINPROGRESS || e == WSAEWOULDBLOCK;
#else
	return e == EINTR || e == EINPROGRESS || e == EAGAIN;
#endif
}

/**
 * Connects the socket to the specified host and port.
 *
 * @return the socket file descriptor, or -1 on failure
 */
mpd_socket_t
mpd_socket_connect(const char *host, unsigned port, const struct timeval *tv,
		   struct mpd_error_info *error);

/**
 * Closes a socket descriptor.  This is a wrapper for close() or
 * closesocket(), depending on the OS.
 */
int
mpd_socket_close(mpd_socket_t fd);

/**
 * Sets (or unsets) keepalive on a socket descriptor.
 */
int
mpd_socket_keepalive(mpd_socket_t fd, bool keepalive);

#endif
