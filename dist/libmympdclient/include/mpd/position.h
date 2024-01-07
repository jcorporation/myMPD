// SPDX-License-Identifier: BSD-3-Clause
// Copyright The Music Player Daemon Project

/*! \file
 * \brief MPD client library
 *
 * Do not include this header directly.  Use mpd/client.h instead.
 */

#ifndef LIBMPDCLIENT_POSITION_H
#define LIBMPDCLIENT_POSITION_H

enum mpd_position_whence {
	/**
	 * The given number is an absolute position.  0 is the first
	 * song.
	 */
	MPD_POSITION_ABSOLUTE,

	/**
	 * The given number is a position after the current song.  0
	 * means right after the current song.
	 */
	MPD_POSITION_AFTER_CURRENT,

	/**
	 * The given number is a position before the current song.  0
	 * means right before the current song.
	 */
	MPD_POSITION_BEFORE_CURRENT,
};

/**
 * Looks up the character of the specified whence position.
 *
 * @return the charater, or "" if the whence position is absolute or not valid.
 */
const char *
mpd_position_whence_char(enum mpd_position_whence whence);

#endif
