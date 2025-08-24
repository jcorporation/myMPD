// SPDX-License-Identifier: BSD-2-Clause
// Copyright The Music Player Daemon Project

#include <mpd/binary.h>
#include <mpd/send.h>
#include <mpd/response.h>
#include "isend.h"
#include "run.h"

bool
mpd_send_binarylimit(struct mpd_connection *connection, unsigned limit)
{
	return mpd_send_u_command(connection, "binarylimit", limit);
}

bool
mpd_run_binarylimit(struct mpd_connection *connection, unsigned limit)
{
	return mpd_run_check(connection) &&
		mpd_send_binarylimit(connection, limit) &&
		mpd_response_finish(connection);
}
