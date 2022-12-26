/* libmpdclient
   (c) 2003-2019 The Music Player Daemon Project
   This project's homepage is: http://www.musicpd.org

   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions
   are met:

   - Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.

   - Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.

   - Neither the name of the Music Player Daemon nor the names of its
   contributors may be used to endorse or promote products derived from
   this software without specific prior written permission.

   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
   ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
   A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR
   CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
   EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
   PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
   PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
   LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
   NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
   SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

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
 * Sends the "replay_gain_status" command to MPD. Call mpd_recv_pair() to
 * read response lines. The pair's name should be "replay_gain_mode". Use
 * mpd_parse_replay_gain_name() to check each pair's value.
 *
 * @param connection the connection to MPD
 * @return true on success, false on error
 *
 * @since MPD 0.16, libmpdclient 2.19.
 */
bool
mpd_send_replay_gain_status(struct mpd_connection *connection);

/**
 * Shortcut for mpd_send_replay_gain_status(), mpd_recv_pair_named() and
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
