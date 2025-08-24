// SPDX-License-Identifier: BSD-2-Clause
// Copyright The Music Player Daemon Project

#include <mpd/player.h>
#include <mpd/send.h>
#include <mpd/song.h>
#include <mpd/response.h>
#include "isend.h"
#include "run.h"
#include "config.h" // for HAVE_USELOCALE

#include <limits.h>
#include <stdio.h>

#ifdef HAVE_USELOCALE
#include <locale.h>
#endif

bool
mpd_send_current_song(struct mpd_connection *connection)
{
	return mpd_send_command(connection, "currentsong", NULL);
}

struct mpd_song *
mpd_run_current_song(struct mpd_connection *connection)
{
	struct mpd_song *song;

	if (!mpd_run_check(connection) || !mpd_send_current_song(connection))
		return NULL;

	song = mpd_recv_song(connection);
	if (song == NULL)
		return NULL;

	if (!mpd_response_finish(connection)) {
		mpd_song_free(song);
		return NULL;
	}

	return song;
}

bool
mpd_send_play(struct mpd_connection *connection)
{
	return mpd_send_command(connection, "play", NULL);
}

bool
mpd_run_play(struct mpd_connection *connection)
{
	return mpd_run_check(connection) && mpd_send_play(connection) &&
		mpd_response_finish(connection);
}

bool
mpd_send_play_pos(struct mpd_connection *connection, unsigned song_pos)
{
	return mpd_send_u_command(connection, "play", song_pos);
}

bool
mpd_run_play_pos(struct mpd_connection *connection, unsigned song_pos)
{
	return mpd_run_check(connection) &&
		mpd_send_play_pos(connection, song_pos) &&
		mpd_response_finish(connection);
}

bool
mpd_send_play_id(struct mpd_connection *connection, unsigned song_id)
{
	return mpd_send_u_command(connection, "playid", song_id);
}

bool
mpd_run_play_id(struct mpd_connection *connection, unsigned song_id)
{
	return mpd_run_check(connection) &&
		mpd_send_play_id(connection, song_id) &&
		mpd_response_finish(connection);
}

bool
mpd_send_stop(struct mpd_connection *connection)
{
	return mpd_send_command(connection, "stop", NULL);
}

bool
mpd_run_stop(struct mpd_connection *connection)
{
	return mpd_run_check(connection) && mpd_send_stop(connection) &&
		mpd_response_finish(connection);
}

bool
mpd_send_toggle_pause(struct mpd_connection *connection)
{
	return mpd_send_command(connection, "pause", NULL);
}

bool
mpd_run_toggle_pause(struct mpd_connection *connection)
{
	return mpd_run_check(connection) &&
		mpd_send_toggle_pause(connection) &&
		mpd_response_finish(connection);
}

bool
mpd_send_pause(struct mpd_connection *connection, bool mode)
{
	return mpd_send_int_command(connection, "pause", mode);
}

bool
mpd_run_pause(struct mpd_connection *connection, bool mode)
{
	return mpd_run_check(connection) && mpd_send_pause(connection, mode) &&
		mpd_response_finish(connection);
}

bool
mpd_send_next(struct mpd_connection *connection)
{
	return mpd_send_command(connection, "next", NULL);
}

bool
mpd_run_next(struct mpd_connection *connection)
{
	return mpd_run_check(connection) && mpd_send_next(connection) &&
		mpd_response_finish(connection);
}

bool
mpd_send_previous(struct mpd_connection *connection)
{
	return mpd_send_command(connection, "previous", NULL);
}

bool
mpd_run_previous(struct mpd_connection *connection)
{
	return mpd_run_check(connection) && mpd_send_previous(connection) &&
		mpd_response_finish(connection);
}

bool
mpd_send_seek_pos(struct mpd_connection *connection,
		 unsigned song_pos, unsigned t)
{
	return mpd_send_u2_command(connection, "seek", song_pos, t);
}

bool
mpd_run_seek_pos(struct mpd_connection *connection,
		unsigned song_pos, unsigned t)
{
	return mpd_run_check(connection) &&
		mpd_send_seek_pos(connection, song_pos, t) &&
		mpd_response_finish(connection);
}

bool
mpd_send_seek_id(struct mpd_connection *connection,
		 unsigned song_id, unsigned t)
{
	return mpd_send_u2_command(connection, "seekid", song_id, t);
}

bool
mpd_run_seek_id(struct mpd_connection *connection,
	       unsigned song_id, unsigned t)
{
	return mpd_run_check(connection) &&
		mpd_send_seek_id(connection, song_id, t) &&
		mpd_response_finish(connection);
}

bool
mpd_send_seek_id_float(struct mpd_connection *connection,
		       unsigned song_id, float t)
{
	return mpd_send_u_f_command(connection, "seekid", song_id, t);
}

bool
mpd_run_seek_id_float(struct mpd_connection *connection,
		      unsigned song_id, float t)
{
	return mpd_run_check(connection) &&
		mpd_send_seek_id_float(connection, song_id, t) &&
		mpd_response_finish(connection);
}

bool
mpd_send_seek_current(struct mpd_connection *connection,
		      float t, bool relative)
{
#ifdef HAVE_USELOCALE
	// use the POSIX locale to format floating point numbers
	const locale_t my_locale = newlocale(LC_NUMERIC_MASK, "C", NULL);
	const locale_t old_locale = uselocale(my_locale);
#endif

	char ts[32];
	if (relative)
		snprintf(ts, sizeof(ts), "%+.3f", (double)t);
	else
		snprintf(ts, sizeof(ts), "%.3f", (double)t);

#ifdef HAVE_USELOCALE
	uselocale(old_locale);
	freelocale(my_locale);
#endif

	return mpd_send_command(connection, "seekcur", ts, NULL);
}

bool
mpd_run_seek_current(struct mpd_connection *connection,
		     float t, bool relative)
{
	return mpd_run_check(connection) &&
		mpd_send_seek_current(connection, t, relative) &&
		mpd_response_finish(connection);
}

bool
mpd_send_repeat(struct mpd_connection *connection, bool mode)
{
	return mpd_send_int_command(connection, "repeat", mode);
}

bool
mpd_run_repeat(struct mpd_connection *connection, bool mode)
{
	return mpd_run_check(connection) &&
		mpd_send_repeat(connection, mode) &&
		mpd_response_finish(connection);
}

bool
mpd_send_random(struct mpd_connection *connection, bool mode)
{
	return mpd_send_int_command(connection, "random", mode);
}

bool
mpd_run_random(struct mpd_connection *connection, bool mode)
{
	return mpd_run_check(connection) &&
		mpd_send_random(connection, mode) &&
		mpd_response_finish(connection);
}

bool
mpd_send_single_state(struct mpd_connection *connection,
		      enum mpd_single_state state)
{
	const char *state_str = mpd_lookup_single_state(state);
	if (state_str == NULL)
		return false;

	return mpd_send_command(connection, "single", state_str, NULL);
}

bool
mpd_run_single_state(struct mpd_connection *connection,
		      enum mpd_single_state state)
{
	return mpd_run_check(connection) &&
		mpd_send_single_state(connection, state) &&
		mpd_response_finish(connection);
}

bool
mpd_send_single(struct mpd_connection *connection, bool mode)
{
	return mpd_send_int_command(connection, "single", mode);
}

bool
mpd_run_single(struct mpd_connection *connection, bool mode)
{
	return mpd_run_check(connection) &&
		mpd_send_single(connection, mode) &&
		mpd_response_finish(connection);
}

bool
mpd_send_consume_state(struct mpd_connection *connection,
		      enum mpd_consume_state state)
{
	const char *state_str = mpd_lookup_consume_state(state);
	if (state_str == NULL)
		return false;

	return mpd_send_command(connection, "consume", state_str, NULL);
}

bool
mpd_run_consume_state(struct mpd_connection *connection,
		      enum mpd_consume_state state)
{
	return mpd_run_check(connection) &&
		mpd_send_consume_state(connection, state) &&
		mpd_response_finish(connection);
}

bool
mpd_send_consume(struct mpd_connection *connection, bool mode)
{
	return mpd_send_int_command(connection, "consume", mode);
}

bool
mpd_run_consume(struct mpd_connection *connection, bool mode)
{
	return mpd_run_check(connection) &&
		mpd_send_consume(connection, mode) &&
		mpd_response_finish(connection);
}

bool
mpd_send_crossfade(struct mpd_connection *connection, unsigned seconds)
{
	return mpd_send_u_command(connection, "crossfade", seconds);
}

bool
mpd_run_crossfade(struct mpd_connection *connection, unsigned seconds)
{
	return mpd_run_check(connection) &&
		mpd_send_crossfade(connection, seconds) &&
		mpd_response_finish(connection);
}

bool
mpd_send_mixrampdb(struct mpd_connection *connection, float db)
{
	return mpd_send_float_command(connection, "mixrampdb", db);
}

bool
mpd_run_mixrampdb(struct mpd_connection *connection, float db)
{
	return mpd_run_check(connection) &&
		mpd_send_mixrampdb(connection, db) &&
		mpd_response_finish(connection);
}

bool
mpd_send_mixrampdelay(struct mpd_connection *connection, float seconds)
{
	return mpd_send_float_command(connection, "mixrampdelay", seconds);
}

bool
mpd_run_mixrampdelay(struct mpd_connection *connection, float seconds)
{
	return mpd_run_check(connection) &&
		mpd_send_mixrampdelay(connection, seconds) &&
		mpd_response_finish(connection);
}

bool
mpd_send_clearerror(struct mpd_connection *connection)
{
	return mpd_send_command(connection, "clearerror", NULL);
}

bool
mpd_run_clearerror(struct mpd_connection *connection)
{
	return mpd_run_check(connection) && mpd_send_clearerror(connection) &&
		mpd_response_finish(connection);
}
