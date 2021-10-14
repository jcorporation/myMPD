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
mpd_run_replay_gain_status(struct mpd_connection *connection)
{
	enum mpd_replay_gain_mode mode;
	struct mpd_pair *pair;

	if (!mpd_run_check(connection) ||
	    !mpd_send_replay_gain_status(connection))
		return MPD_REPLAY_UNKNOWN;

	pair = mpd_recv_pair_named(connection, "replay_gain_mode");
	if (pair != NULL) {
		mode = mpd_parse_replay_gain_name(pair->value);
		mpd_return_pair(connection, pair);
	} else
		mode = MPD_REPLAY_UNKNOWN;

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
