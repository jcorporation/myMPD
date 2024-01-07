// SPDX-License-Identifier: BSD-2-Clause
// Copyright The Music Player Daemon Project

#include <mpd/connection.h>
#include "internal.h"

#include <assert.h>

enum mpd_error
mpd_connection_get_error(const struct mpd_connection *connection)
{
	return connection->error.code;
}

const char *
mpd_connection_get_error_message(const struct mpd_connection *connection)
{
	return mpd_error_get_message(&connection->error);
}

enum mpd_server_error
mpd_connection_get_server_error(const struct mpd_connection *connection)
{
	assert(connection->error.code == MPD_ERROR_SERVER);

	return connection->error.server;
}

unsigned
mpd_connection_get_server_error_location(const struct mpd_connection *connection)
{
	assert(connection->error.code == MPD_ERROR_SERVER);

	return connection->error.at;
}

int
mpd_connection_get_system_error(const struct mpd_connection *connection)
{
	assert(connection->error.code == MPD_ERROR_SYSTEM);

	return connection->error.system;
}

bool
mpd_connection_clear_error(struct mpd_connection *connection)
{
	if (mpd_error_is_fatal(&connection->error))
		/* impossible to recover */
		return false;

	mpd_error_clear(&connection->error);
	return true;
}
