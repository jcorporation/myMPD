// SPDX-License-Identifier: BSD-2-Clause
// Copyright The Music Player Daemon Project

#include "run.h"
#include <mpd/response.h>
#include <mpd/status.h>
#include "internal.h"

/**
 * Checks whether it is possible to run a command now.
 */
bool
mpd_run_check(struct mpd_connection *connection)
{
	assert(connection != NULL);

	if (mpd_error_is_defined(&connection->error))
		return false;

	if (connection->sending_command_list) {
		mpd_error_code(&connection->error, MPD_ERROR_STATE);
		mpd_error_message(&connection->error,
				  "Not possible in command list mode");
		return false;
	}

	return true;
}
