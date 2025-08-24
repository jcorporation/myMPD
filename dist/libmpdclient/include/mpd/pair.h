// SPDX-License-Identifier: BSD-2-Clause
// Copyright The Music Player Daemon Project

/*! \file
 * \brief MPD client library
 *
 * Do not include this header directly.  Use mpd/client.h instead.
 */

#ifndef LIBMPDCLIENT_PAIR_H
#define LIBMPDCLIENT_PAIR_H

/**
 * A name-value pair received from the MPD server.
 */
struct mpd_pair {
	/** the name of the element */
	const char *name;

	/** the value of the element */
	const char *value;
};

#endif
