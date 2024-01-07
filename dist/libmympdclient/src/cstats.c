// SPDX-License-Identifier: BSD-2-Clause
// Copyright The Music Player Daemon Project

/*
 * mpd_connection specific functions declared in mpd/stats.h.
 *
 */

#include <mpd/stats.h>
#include <mpd/send.h>
#include <mpd/recv.h>
#include "internal.h"

#include <assert.h>

bool
mpd_send_stats(struct mpd_connection *connection)
{
	return mpd_send_command(connection, "stats", NULL);
}

struct mpd_stats *
mpd_recv_stats(struct mpd_connection *connection)
{
	struct mpd_stats * stats;
	struct mpd_pair *pair;

	assert(connection != NULL);

	if (mpd_error_is_defined(&connection->error))
		/* refuse to receive a response if the connection's
		   state is not clean */
		return NULL;

	stats = mpd_stats_begin();
	if (stats == NULL) {
		mpd_error_code(&connection->error, MPD_ERROR_OOM);
		return NULL;
	}

	/* read and parse all response lines */
	while ((pair = mpd_recv_pair(connection)) != NULL) {
		mpd_stats_feed(stats, pair);
		mpd_return_pair(connection, pair);
	}

	if (mpd_error_is_defined(&connection->error)) {
		/* an error has occurred; roll back */
		mpd_stats_free(stats);
		return NULL;
	}

	return stats;
}


struct mpd_stats *
mpd_run_stats(struct mpd_connection *connection)
{
	return mpd_send_stats(connection)
		? mpd_recv_stats(connection)
		: NULL;
}
