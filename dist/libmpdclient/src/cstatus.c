// SPDX-License-Identifier: BSD-2-Clause
// Copyright The Music Player Daemon Project

/*
 * mpd_connection specific functions declared in mpd/status.h.
 *
 */

#include <mpd/status.h>
#include <mpd/send.h>
#include <mpd/recv.h>
#include "internal.h"
#include "run.h"

bool
mpd_send_status(struct mpd_connection * connection)
{
	return mpd_send_command(connection, "status", NULL);
}

struct mpd_status *
mpd_recv_status(struct mpd_connection * connection)
{
	struct mpd_status * status;
	struct mpd_pair *pair;

	if (mpd_error_is_defined(&connection->error))
		return NULL;

	status = mpd_status_begin();
	if (status == NULL) {
		mpd_error_code(&connection->error, MPD_ERROR_OOM);
		return NULL;
	}

	while ((pair = mpd_recv_pair(connection)) != NULL) {
		mpd_status_feed(status, pair);
		mpd_return_pair(connection, pair);
	}

	if (mpd_error_is_defined(&connection->error)) {
		mpd_status_free(status);
		return NULL;
	}

	return status;
}

struct mpd_status *
mpd_run_status(struct mpd_connection *connection)
{
	return mpd_run_check(connection) && mpd_send_status(connection)
		? mpd_recv_status(connection)
		: NULL;
}
