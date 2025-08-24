// SPDX-License-Identifier: BSD-2-Clause
// Copyright The Music Player Daemon Project

/*! \file
 * \brief MPD client library
 *
 * Do not include this header directly.  Use mpd/client.h or
 * mpd/async.h instead.
 */

#ifndef MPD_ERROR_H
#define MPD_ERROR_H

enum mpd_error {
	/** no error */
	MPD_ERROR_SUCCESS = 0,

	/** out of memory */
	MPD_ERROR_OOM,

	/** a function was called with an unrecognized or invalid
	    argument */
	MPD_ERROR_ARGUMENT,

	/** a function was called which is not available in the
	    current state of libmpdclient */
	MPD_ERROR_STATE,

	/** timeout trying to talk to mpd */
	MPD_ERROR_TIMEOUT,

	/** system error */
	MPD_ERROR_SYSTEM,

	/** unknown host */
	MPD_ERROR_RESOLVER,

	/** malformed response received from MPD */
	MPD_ERROR_MALFORMED,

	/** connection closed by mpd */
	MPD_ERROR_CLOSED,

	/**
	 * The server has returned an error code, which can be queried
	 * with mpd_connection_get_server_error().
	 */
	MPD_ERROR_SERVER,
};

#endif
