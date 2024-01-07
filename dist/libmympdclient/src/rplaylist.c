// SPDX-License-Identifier: BSD-2-Clause
// Copyright The Music Player Daemon Project

#include <mpd/playlist.h>
#include <mpd/recv.h>
#include "internal.h"

#include <errno.h>

struct mpd_playlist *
mpd_recv_playlist(struct mpd_connection *connection)
{
	struct mpd_pair *pair;
	struct mpd_playlist *playlist;

	pair = mpd_recv_pair_named(connection, "playlist");
	if (pair == NULL)
		return NULL;

	playlist = mpd_playlist_begin(pair);
	mpd_return_pair(connection, pair);
	if (playlist == NULL) {
		mpd_error_entity(&connection->error);
		return NULL;
	}

	while ((pair = mpd_recv_pair(connection)) != NULL &&
	       mpd_playlist_feed(playlist, pair))
		mpd_return_pair(connection, pair);

	if (mpd_error_is_defined(&connection->error)) {
		assert(pair == NULL);

		mpd_playlist_free(playlist);
		return NULL;
	}

	/* unread this pair for the next mpd_recv_playlist() call */
	mpd_enqueue_pair(connection, pair);

	return playlist;
}
