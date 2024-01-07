// SPDX-License-Identifier: BSD-3-Clause
// Copyright The Music Player Daemon Project

#include <mpd/response.h>
#include <mpd/recv.h>
#include "internal.h"

#include <assert.h>

bool
mpd_response_finish(struct mpd_connection *connection)
{
	struct mpd_pair *pair;

	if (mpd_error_is_defined(&connection->error))
		return false;

	if (connection->pair_state == PAIR_STATE_NULL)
		/* reset the stored NULL pair because it will conflict
		   with an assertion within the loop */
		connection->pair_state = PAIR_STATE_NONE;

	while (connection->receiving) {
		assert(!mpd_error_is_defined(&connection->error));

		connection->discrete_finished = false;

		pair = mpd_recv_pair(connection);
		assert(pair != NULL || !connection->receiving ||
		       (connection->sending_command_list &&
			connection->discrete_finished) ||
		       mpd_error_is_defined(&connection->error));

		if (pair != NULL)
			mpd_return_pair(connection, pair);
	}

	return !mpd_error_is_defined(&connection->error);
}

bool
mpd_response_next(struct mpd_connection *connection)
{
	struct mpd_pair *pair;

	if (mpd_error_is_defined(&connection->error))
		return false;

	if (!connection->receiving) {
		mpd_error_code(&connection->error, MPD_ERROR_STATE);
		mpd_error_message(&connection->error,
				  "Response is already finished");
		return false;
	}

	if (!connection->sending_command_list_ok) {
		mpd_error_code(&connection->error, MPD_ERROR_STATE);
		mpd_error_message(&connection->error,
				  "Not in command list mode");
		return false;
	}

	while (!connection->discrete_finished) {
		if (connection->command_list_remaining == 0 ||
		    !connection->receiving) {
			mpd_error_code(&connection->error,
				       MPD_ERROR_MALFORMED);
			mpd_error_message(&connection->error,
					  "No list_OK found");
			return false;
		}

		pair = mpd_recv_pair(connection);
		if (pair != NULL)
			mpd_return_pair(connection, pair);
		else if (mpd_error_is_defined(&connection->error))
			return false;
	}

	connection->discrete_finished = false;
	return true;
}
