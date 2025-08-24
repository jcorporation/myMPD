// SPDX-License-Identifier: BSD-2-Clause
// Copyright The Music Player Daemon Project

#include <mpd/neighbor.h>
#include <mpd/send.h>
#include <mpd/recv.h>
#include <mpd/response.h>
#include "internal.h"

#include <stddef.h>

bool
mpd_send_list_neighbors(struct mpd_connection *connection)
{
	return mpd_send_command(connection, "listneighbors", NULL);
}

struct mpd_neighbor *
mpd_recv_neighbor(struct mpd_connection *connection)
{
	struct mpd_neighbor *neighbor;
	struct mpd_pair *pair;

	pair = mpd_recv_pair_named(connection, "neighbor");
	if (pair == NULL)
		return NULL;

	neighbor = mpd_neighbor_begin(pair);
	mpd_return_pair(connection, pair);
	if (neighbor == NULL) {
		mpd_error_code(&connection->error, MPD_ERROR_OOM);
		return NULL;
	}

	while ((pair = mpd_recv_pair(connection)) != NULL &&
	       mpd_neighbor_feed(neighbor, pair))
		mpd_return_pair(connection, pair);

	if (mpd_error_is_defined(&connection->error)) {
		assert(pair == NULL);

		mpd_neighbor_free(neighbor);
		return NULL;
	}

	mpd_enqueue_pair(connection, pair);
	return neighbor;
}
