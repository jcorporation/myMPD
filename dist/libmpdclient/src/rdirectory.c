// SPDX-License-Identifier: BSD-2-Clause
// Copyright The Music Player Daemon Project

#include <mpd/directory.h>
#include <mpd/recv.h>
#include "internal.h"

#include <errno.h>

struct mpd_directory *
mpd_recv_directory(struct mpd_connection *connection)
{
	struct mpd_pair *pair;
	struct mpd_directory *directory;

	pair = mpd_recv_pair_named(connection, "directory");
	if (pair == NULL)
		return NULL;

	directory = mpd_directory_begin(pair);
	mpd_return_pair(connection, pair);
	if (directory == NULL) {
		mpd_error_entity(&connection->error);
		return NULL;
	}

	while ((pair = mpd_recv_pair(connection)) != NULL &&
	       mpd_directory_feed(directory, pair))
		mpd_return_pair(connection, pair);

	if (mpd_error_is_defined(&connection->error)) {
		assert(pair == NULL);

		mpd_directory_free(directory);
		return NULL;
	}

	/* unread this pair for the next mpd_recv_directory() call */
	mpd_enqueue_pair(connection, pair);

	return directory;
}
