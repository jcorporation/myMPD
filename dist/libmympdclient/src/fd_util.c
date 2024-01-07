// SPDX-License-Identifier: BSD-2-Clause
// Copyright The Music Player Daemon Project

/*
 * This code is copied from MPD.  It is a subset of the original
 * library (we don't need pipes and regular files in libmpdclient).
 *
 */

#include "fd_util.h"

#include <assert.h>
#include <stdbool.h>
#include <fcntl.h>
#include <errno.h>

#ifdef _WIN32
#include <winsock2.h>
#else
#include <sys/socket.h>
#endif

#ifndef _WIN32

static int
fd_mask_flags(mpd_socket_t fd, int and_mask, int xor_mask)
{
	int ret;

	assert(fd != MPD_INVALID_SOCKET);

	ret = fcntl(fd, F_GETFD, 0);
	if (ret < 0)
		return ret;

	return fcntl(fd, F_SETFD, (ret & and_mask) ^ xor_mask);
}

#endif /* !_WIN32 */

static int
fd_set_cloexec(mpd_socket_t fd, bool enable)
{
#ifndef _WIN32
	return fd_mask_flags(fd, ~FD_CLOEXEC, enable ? FD_CLOEXEC : 0);
#else
	(void)fd;
	(void)enable;
	return 0;
#endif
}

/**
 * Enables non-blocking mode for the specified file descriptor.  On
 * WIN32, this function only works for sockets.
 */
static int
fd_set_nonblock(mpd_socket_t fd)
{
#ifdef _WIN32
	u_long val = 1;
	return ioctlsocket(fd, FIONBIO, &val);
#else
	int flags;

	assert(fd != MPD_INVALID_SOCKET);

	flags = fcntl(fd, F_GETFL);
	if (flags < 0)
		return flags;

	return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
#endif
}

mpd_socket_t
socket_cloexec_nonblock(int domain, int type, int protocol)
{
	mpd_socket_t fd;

#if defined(SOCK_CLOEXEC) && defined(SOCK_NONBLOCK)
	fd = socket(domain, type | SOCK_CLOEXEC | SOCK_NONBLOCK, protocol);
	if (fd != MPD_INVALID_SOCKET || errno != EINVAL)
		return fd;
#endif

	fd = socket(domain, type, protocol);
	if (fd != MPD_INVALID_SOCKET) {
		fd_set_cloexec(fd, true);
		fd_set_nonblock(fd);
	}

	return fd;
}
