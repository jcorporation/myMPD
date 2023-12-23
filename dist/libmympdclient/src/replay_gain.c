// SPDX-License-Identifier: BSD-3-Clause
// Copyright The Music Player Daemon Project

#include <mpd/replay_gain.h>
#include <mpd/send.h>
#include <mpd/recv.h>
#include <mpd/pair.h>
#include <mpd/response.h>
#include "run.h"

#include <string.h>
#include <stdio.h>

enum mpd_replay_gain_mode
mpd_parse_replay_gain_name(const char *name)
{
	if (strcmp(name, "off") == 0)
		return MPD_REPLAY_OFF;
	else if (strcmp(name, "track") == 0)
		return MPD_REPLAY_TRACK;
	else if (strcmp(name, "album") == 0)
		return MPD_REPLAY_ALBUM;
	else if (strcmp(name, "auto") == 0)
		return MPD_REPLAY_AUTO;
	else
		return MPD_REPLAY_UNKNOWN;
}

bool
mpd_send_replay_gain_status(struct mpd_connection *connection)
{
	return mpd_send_command(connection, "replay_gain_status", NULL);
}

const char *
mpd_lookup_replay_gain_mode(enum mpd_replay_gain_mode mode)
{
	switch (mode) {
	case MPD_REPLAY_OFF:
		return "off";
	case MPD_REPLAY_TRACK:
		return "track";
	case MPD_REPLAY_ALBUM:
		return "album";
	case MPD_REPLAY_AUTO:
		return "auto";
	case MPD_REPLAY_UNKNOWN:
		return NULL;
	}

	return NULL;
}

enum mpd_replay_gain_mode
mpd_recv_replay_gain_status(struct mpd_connection *connection)
{
	enum mpd_replay_gain_mode mode;
	struct mpd_pair *pair;

	pair = mpd_recv_pair_named(connection, "replay_gain_mode");
	if (pair != NULL) {
		mode = mpd_parse_replay_gain_name(pair->value);
		mpd_return_pair(connection, pair);
	} else
		mode = MPD_REPLAY_UNKNOWN;

	return mode;
}

enum mpd_replay_gain_mode
mpd_run_replay_gain_status(struct mpd_connection *connection)
{
	enum mpd_replay_gain_mode mode;

	if (!mpd_run_check(connection) ||
	    !mpd_send_replay_gain_status(connection))
		return MPD_REPLAY_UNKNOWN;

	mode = mpd_recv_replay_gain_status(connection);

	if (!mpd_response_finish(connection))
		return MPD_REPLAY_UNKNOWN;

	return mode;
}

bool
mpd_send_replay_gain_mode(struct mpd_connection *connection,
			  enum mpd_replay_gain_mode mode)
{
	return mpd_send_command(connection, "replay_gain_mode",
			        mpd_lookup_replay_gain_mode(mode), NULL);
}

bool
mpd_run_replay_gain_mode(struct mpd_connection *connection,
			  enum mpd_replay_gain_mode mode)
{
	return mpd_run_check(connection) &&
		mpd_send_replay_gain_mode(connection, mode) &&
		mpd_response_finish(connection);

}
