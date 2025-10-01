// SPDX-License-Identifier: BSD-2-Clause
// Copyright The Music Player Daemon Project

/*! \file
 * \brief Synchronous MPD connections
 *
 * This library provides synchronous access to a mpd_async object.
 * For all operations, you may provide a timeout.
 */

#ifndef MPD_SYNC_H
#define MPD_SYNC_H

#include <mpd/compiler.h>

#include <stdbool.h>
#include <stdarg.h>
#include <stddef.h>

struct timeval;
struct mpd_async;

/**
 * Synchronous wrapper for mpd_async_send_command_v().
 */
bool
mpd_sync_send_command_v(struct mpd_async *async, const struct timeval *tv,
			const char *command, va_list args);

/**
 * Synchronous wrapper for mpd_async_send_command().
 */
mpd_sentinel
bool
mpd_sync_send_command(struct mpd_async *async, const struct timeval *tv,
		      const char *command, ...);

/**
 * Sends all pending data from the output buffer to MPD.
 */
bool
mpd_sync_flush(struct mpd_async *async, const struct timeval *tv);

/**
 * Synchronous wrapper for mpd_async_recv_line().
 */
char *
mpd_sync_recv_line(struct mpd_async *async, const struct timeval *tv);

/**
 * Synchronous wrapper for mpd_async_recv_raw() which waits until at
 * least one byte was received (or an error has occurred).
 *
 * @return the number of bytes copied to the destination buffer or 0
 * on error
 */
size_t
mpd_sync_recv_raw(struct mpd_async *async, const struct timeval *tv,
		  void *dest, size_t length);

#endif
