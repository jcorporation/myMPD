// SPDX-License-Identifier: BSD-2-Clause
// Copyright The Music Player Daemon Project

/*
 * This library provides easy helper functions for working with file
 * descriptors.  It has wrappers for taking advantage of Linux 2.6
 * specific features like O_CLOEXEC.
 *
 */

#ifndef FD_UTIL_H
#define FD_UTIL_H

#include <mpd/socket.h>

/**
 * Wrapper for socket(), which sets the CLOEXEC and the NONBLOCK flag
 * (atomically if supported by the OS).
 */
mpd_socket_t
socket_cloexec_nonblock(int domain, int type, int protocol);

#endif
