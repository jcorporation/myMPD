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
 * Controlling playback.
 *
 * Do not include this header directly.  Use mpd/client.h instead.
 */

#ifndef MPD_PLAYER_H
#define MPD_PLAYER_H

#include "compiler.h"

#include <stdbool.h>

struct mpd_connection;
struct mpd_song;

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Fetches the currently selected song (the song referenced by
 * status->song and status->songid).
 */
bool
mpd_send_current_song(struct mpd_connection *connection);

/**
 * Shortcut for mpd_send_currentsong() and mpd_recv_song().
 *
 * @param connection the connection to MPD
 * @return the current song, or NULL on error or if there is no
 * current song
 */
mpd_malloc
struct mpd_song *
mpd_run_current_song(struct mpd_connection *connection);

/**
 * Starts playing the current song from the beginning.
 *
 * @param connection the connection to MPD
 */
bool
mpd_send_play(struct mpd_connection *connection);

bool
mpd_run_play(struct mpd_connection *connection);

/**
 * Starts playing the specified song from the beginning.
 *
 * @param song_pos the position of the song in the queue
 * @param connection the connection to MPD
 * @return true on success, false on error
 */
bool
mpd_send_play_pos(struct mpd_connection *connection, unsigned song_pos);

bool
mpd_run_play_pos(struct mpd_connection *connection, unsigned song_pos);

/**
 * Starts playing the specified song from the beginning.
 *
 * @param connection the connection to MPD
 * @param id the id of the song
 * @return true on success, false on error
 */
bool
mpd_send_play_id(struct mpd_connection *connection, unsigned id);

bool
mpd_run_play_id(struct mpd_connection *connection, unsigned song_id);

bool
mpd_send_stop(struct mpd_connection *connection);

bool
mpd_run_stop(struct mpd_connection *connection);

/**
 * Toggles the pause mode by sending "pause" without arguments.
 *
 * @param connection the connection to MPD
 */
bool
mpd_send_toggle_pause(struct mpd_connection *connection);

bool
mpd_run_toggle_pause(struct mpd_connection *connection);

bool
mpd_send_pause(struct mpd_connection *connection, bool mode);

bool
mpd_run_pause(struct mpd_connection *connection, bool mode);

bool
mpd_send_next(struct mpd_connection *connection);

bool
mpd_run_next(struct mpd_connection *connection);

bool
mpd_send_previous(struct mpd_connection *connection);

bool
mpd_run_previous(struct mpd_connection *connection);

/**
 * Seeks the specified song.
 *
 * @param connection the connection to MPD
 * @param song_pos the position of the song in the queue
 * @param t the position within the song, in seconds
 * @return true on success, false on error
 */
bool
mpd_send_seek_pos(struct mpd_connection *connection,
		  unsigned song_pos, unsigned t);

bool
mpd_run_seek_pos(struct mpd_connection *connection,
		 unsigned song_pos, unsigned t);

/**
 * Seeks the specified song.
 *
 * @param connection the connection to MPD
 * @param id the id of the song
 * @param t the position within the song, in seconds
 * @return true on success, false on error
 */
bool
mpd_send_seek_id(struct mpd_connection *connection, unsigned id, unsigned t);

bool
mpd_run_seek_id(struct mpd_connection *connection,
		unsigned song_id, unsigned t);

/**
 * Seeks the specified song (with floating point time).
 *
 * @param connection the connection to MPD
 * @param id the id of the song
 * @param t the position within the song, in seconds
 * @return true on success, false on error
 */
bool
mpd_send_seek_id_float(struct mpd_connection *connection,
		       unsigned id, float t);

bool
mpd_run_seek_id_float(struct mpd_connection *connection,
		      unsigned song_id, float t);

/**
 * Seeks the current song.
 *
 * @param connection the connection to MPD
 * @param t the position within the song, in seconds
 * @param relative true makes #t a relative to the current position
 * @return true on success, false on error
 *
 * @since MPD 0.17, libmpdclient 2.15
 */
bool
mpd_send_seek_current(struct mpd_connection *connection,
		      float t, bool relative);

bool
mpd_run_seek_current(struct mpd_connection *connection,
		     float t, bool relative);

bool
mpd_send_repeat(struct mpd_connection *connection, bool mode);

bool
mpd_run_repeat(struct mpd_connection *connection, bool mode);

bool
mpd_send_random(struct mpd_connection *connection, bool mode);

bool
mpd_run_random(struct mpd_connection *connection, bool mode);

bool
mpd_send_single(struct mpd_connection *connection, bool mode);

bool
mpd_run_single(struct mpd_connection *connection, bool mode);

bool
mpd_send_consume(struct mpd_connection *connection, bool mode);

bool
mpd_run_consume(struct mpd_connection *connection, bool mode);

bool
mpd_send_crossfade(struct mpd_connection *connection, unsigned seconds);

bool
mpd_run_crossfade(struct mpd_connection *connection, unsigned seconds);

/**
 * @since libmpdclient 2.2
 */
bool
mpd_send_mixrampdb(struct mpd_connection *connection, float db);

/**
 * @since libmpdclient 2.2
 */
bool
mpd_run_mixrampdb(struct mpd_connection *connection, float db);

/**
 * @since libmpdclient 2.2
 */
bool
mpd_send_mixrampdelay(struct mpd_connection *connection, float seconds);

/**
 * @since libmpdclient 2.2
 */
bool
mpd_run_mixrampdelay(struct mpd_connection *connection, float seconds);

/**
 * @since libmpdclient 2.4
 */
bool
mpd_send_clearerror(struct mpd_connection *connection);

/**
 * @since libmpdclient 2.4
 */
bool
mpd_run_clearerror(struct mpd_connection *connection);

#ifdef __cplusplus
}
#endif

#endif
