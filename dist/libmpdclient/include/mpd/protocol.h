// SPDX-License-Identifier: BSD-2-Clause
// Copyright The Music Player Daemon Project

/*! \file
 * \brief MPD client library
 *
 * Do not include this header directly.  Use mpd/client.h or
 * mpd/parser.h instead.
 */

#ifndef MPD_PROTOCOL_H
#define MPD_PROTOCOL_H

enum mpd_server_error {
	MPD_SERVER_ERROR_UNK = -1,

	MPD_SERVER_ERROR_NOT_LIST = 1,
	MPD_SERVER_ERROR_ARG = 2,
	MPD_SERVER_ERROR_PASSWORD = 3,
	MPD_SERVER_ERROR_PERMISSION = 4,
	MPD_SERVER_ERROR_UNKNOWN_CMD = 5,

	MPD_SERVER_ERROR_NO_EXIST = 50,
	MPD_SERVER_ERROR_PLAYLIST_MAX = 51,
	MPD_SERVER_ERROR_SYSTEM = 52,
	MPD_SERVER_ERROR_PLAYLIST_LOAD = 53,
	MPD_SERVER_ERROR_UPDATE_ALREADY = 54,
	MPD_SERVER_ERROR_PLAYER_SYNC = 55,
	MPD_SERVER_ERROR_EXIST = 56,
};

#endif
