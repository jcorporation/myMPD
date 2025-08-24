// SPDX-License-Identifier: BSD-3-Clause
// Copyright The Music Player Daemon Project

#include <mpd/list.h>
#include <mpd/send.h>
#include "internal.h"
#include "isend.h"

#include <assert.h>

bool
mpd_command_list_begin(struct mpd_connection *connection, bool discrete_ok)
{
	bool success;

	assert(connection != NULL);

	if (connection->sending_command_list) {
		mpd_error_code(&connection->error, MPD_ERROR_STATE);
		mpd_error_message(&connection->error,
				  "already in command list mode");
		return false;
	}

	success = mpd_send_command2(connection,
				    discrete_ok
				    ? "command_list_ok_begin"
				    : "command_list_begin");
	if (!success)
		return false;

	connection->sending_command_list = true;
	connection->sending_command_list_ok = discrete_ok;
	connection->command_list_remaining = 0;
	connection->discrete_finished = false;

	return true;
}

bool
mpd_command_list_end(struct mpd_connection *connection)
{
	bool success;

	assert(connection != NULL);

	if (!connection->sending_command_list) {
		mpd_error_code(&connection->error, MPD_ERROR_STATE);
		mpd_error_message(&connection->error,
				  "not in command list mode");
		return false;
	}

	connection->sending_command_list = false;
	success = mpd_send_command(connection, "command_list_end", NULL);
	/* sending_command_list will be cleared when the user requests the
	   command list response (a function that calls mpd_recv_pair()) */
	connection->sending_command_list = true;
	if (!success)
		return false;

	assert(connection->receiving);
	return true;
}
