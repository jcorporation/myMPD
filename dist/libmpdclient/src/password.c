// SPDX-License-Identifier: BSD-2-Clause
// Copyright The Music Player Daemon Project

#include <mpd/password.h>
#include <mpd/send.h>
#include <mpd/response.h>
#include "run.h"

#include <stddef.h>

bool
mpd_send_password(struct mpd_connection *connection, const char *password)
{
	return mpd_send_command(connection, "password", password, NULL);
}

bool
mpd_run_password(struct mpd_connection *connection, const char *password)
{
	return mpd_run_check(connection) &&
		mpd_send_password(connection, password) &&
		mpd_response_finish(connection);
}

