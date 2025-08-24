// SPDX-License-Identifier: BSD-3-Clause
// Copyright The Music Player Daemon Project

#ifndef MPD_IASYNC_H
#define MPD_IASYNC_H

#include <mpd/async.h>

struct mpd_error_info;

/**
 * Creates a copy of that object's error condition.
 *
 * @return true if there was no error in #async, false if an error
 * condition is stored in #async (in both cases, the information is
 * copied)
 */
bool
mpd_async_copy_error(const struct mpd_async *async,
		     struct mpd_error_info *dest);

/**
 * Sets the object's error condition.
 *
 * @return true if there was no error in #async, false if an error
 * condition is stored in #async; the #error is only set if there was no error
 * previously present
 */
bool
mpd_async_set_error(struct mpd_async *async, enum mpd_error error,
		    const char *error_message);

#endif
