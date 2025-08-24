// SPDX-License-Identifier: BSD-2-Clause
// Copyright The Music Player Daemon Project

#include <mpd/mixer.h>
#include <mpd/send.h>
#include <mpd/recv.h>
#include <mpd/pair.h>
#include <mpd/response.h>
#include "isend.h"
#include "run.h"

#include <stdlib.h>

bool
mpd_send_set_volume(struct mpd_connection *connection, unsigned volume)
{
	return mpd_send_u_command(connection, "setvol", volume);
}

bool
mpd_run_set_volume(struct mpd_connection *connection, unsigned volume)
{
	return mpd_run_check(connection) &&
		mpd_send_set_volume(connection, volume) &&
		mpd_response_finish(connection);
}

bool
mpd_send_change_volume(struct mpd_connection *connection, int relative_volume)
{
	return mpd_send_int_command(connection, "volume", relative_volume);
}

bool
mpd_run_change_volume(struct mpd_connection *connection, int relative_volume)
{
	return mpd_run_check(connection) &&
		mpd_send_change_volume(connection, relative_volume) &&
		mpd_response_finish(connection);
}

bool
mpd_send_get_volume(struct mpd_connection *connection)
{
    return mpd_send_command(connection, "getvol", NULL);
}

int
mpd_run_get_volume(struct mpd_connection *connection)
{
	if (!mpd_run_check(connection) ||
	    !mpd_send_get_volume(connection))
		return -1;

	int result = -1;

	struct mpd_pair *pair = mpd_recv_pair_named(connection, "volume");
	if (pair != NULL) {
		result = atoi(pair->value);
		mpd_return_pair(connection, pair);
	}

	if (!mpd_response_finish(connection))
		result = -1;

	return result;
}
