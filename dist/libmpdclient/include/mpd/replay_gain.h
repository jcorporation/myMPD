// SPDX-License-Identifier: BSD-3-Clause
// Copyright The Music Player Daemon Project

/*! \file
 * \brief MPD client library
 *
 * Do not include this header directly.  Use mpd/client.h instead.
 */

#ifndef MPD_REPLAY_GAIN_H
#define MPD_REPLAY_GAIN_H

#include "compiler.h"

#include <stdbool.h>

struct mpd_connection;

/**
 * MPD's replay gain mode.
 *
 * @since libmpdclient 2.19, MPD 0.16.
 */
enum mpd_replay_gain_mode {
	/** ignore ReplayGain tag values */
	MPD_REPLAY_OFF = 0,

	/** use per-track ReplayGain values */
	MPD_REPLAY_TRACK,

	/** use per-album ReplayGain values */
	MPD_REPLAY_ALBUM,

	/**
	 * use available ReplayGain value; if both track and album tags are
	 * available, prefer track values.
	 **/
	MPD_REPLAY_AUTO,

	/** Unknown mode */
	MPD_REPLAY_UNKNOWN,
};

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Parse a #mpd_pair value to check which replay gain mode it contains.
 */
mpd_pure
enum mpd_replay_gain_mode
mpd_parse_replay_gain_name(const char *name);

/**
 * Looks up the name of the specified replay gain mode.
 *
 * @return the name, or NULL if the replay gain mode is not valid
 *
 * @since libmpdclient 2.19.
 */
mpd_pure
const char *
mpd_lookup_replay_gain_mode(enum mpd_replay_gain_mode mode);

/**
 * Queries the current state of replay gain mode on MPD.
 *
 * Sends the "replay_gain_status" command to MPD.
 * Call mpd_recv_replay_gain_status() to read the response.
 *
 * @param connection the connection to MPD
 * @return true on success, false on error
 *
 * @since MPD 0.16, libmpdclient 2.19.
 */
bool
mpd_send_replay_gain_status(struct mpd_connection *connection);

/**
 * Receives the current state of replay gain mode on MPD.
 * Shortcut for mpd_recv_pair_named() and mpd_parse_replay_gain_name() ad
 * mpd_return_pair().
 *
 * @param connection the connection to MPD
 * @return #mpd_replay_gain_mode object: #MPD_REPLAY_UNKNOWN on error (or
 * unknown ReplayGain mode); other modes on success.
 *
 * @since MPD 0.16, libmpdclient 2.21.
 */
enum mpd_replay_gain_mode
mpd_recv_replay_gain_status(struct mpd_connection *connection);

/**
 * Shortcut for mpd_send_replay_gain_status(), mpd_recv_replay_gain_status() and
 * mpd_response_finish().
 *
 * @param connection the connection to MPD
 * @return #mpd_replay_gain_mode object: #MPD_REPLAY_UNKNOWN on error (or
 * unknown ReplayGain mode); other modes on success.
 *
 * @since MPD 0.16, libmpdclient 2.19.
 */
enum mpd_replay_gain_mode
mpd_run_replay_gain_status(struct mpd_connection *connection);

/**
 * Sets the current state of replay gain mode on MPD.
 *
 * @param connection the connection to MPD
 * @param mode the desired replay gain mode
 * @return true on success, false on error
 *
 * @since MPD 0.16, libmpdclient 2.19.
 */
bool
mpd_send_replay_gain_mode(struct mpd_connection *connection,
			  enum mpd_replay_gain_mode mode);

/**
 * Shortcut for mpd_send_replay_gain_mode() and mpd_response_finish().
 *
 * @param connection the connection to MPD
 * @param mode mode the desired replay gain mode
 * @return true on success, false on error
 *
 * @since MPD 0.16, libmpdclient 2.19.
 */
bool
mpd_run_replay_gain_mode(struct mpd_connection *connection,
			 enum mpd_replay_gain_mode mode);

#ifdef __cplusplus
}
#endif

#endif
