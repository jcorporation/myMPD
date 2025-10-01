// SPDX-License-Identifier: BSD-3-Clause
// Copyright The Music Player Daemon Project

#include <mpd/playlist.h>
#include <mpd/pair.h>
#include "iso8601.h"
#include "uri.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

struct mpd_playlist {
	char *path;

	/**
	 * The POSIX UTC time stamp of the last modification, or 0 if
	 * that is unknown.
	 */
	time_t last_modified;
};

static struct mpd_playlist *
mpd_playlist_new(const char *path)
{
	struct mpd_playlist *playlist;

	assert(path != NULL);
	assert(mpd_verify_local_uri(path));

	playlist = malloc(sizeof(*playlist));
	if (playlist == NULL)
		/* out of memory */
		return NULL;

	playlist->path = strdup(path);
	if (playlist->path == NULL) {
		/* out of memory */
		free(playlist);
		return NULL;
	}

	playlist->last_modified = 0;

	return playlist;
}

void
mpd_playlist_free(struct mpd_playlist *playlist)
{
	assert(playlist != NULL);
	assert(playlist->path != NULL);

	free(playlist->path);
	free(playlist);
}

struct mpd_playlist *
mpd_playlist_dup(const struct mpd_playlist *playlist)
{
	assert(playlist != NULL);
	assert(playlist->path != NULL);

	struct mpd_playlist *copy = mpd_playlist_new(playlist->path);
	copy->last_modified = playlist->last_modified;
	return copy;
}

const char *
mpd_playlist_get_path(const struct mpd_playlist *playlist)
{
	assert(playlist != NULL);

	return playlist->path;
}

time_t
mpd_playlist_get_last_modified(const struct mpd_playlist *playlist)
{
	return playlist->last_modified;
}

struct mpd_playlist *
mpd_playlist_begin(const struct mpd_pair *pair)
{
	assert(pair != NULL);
	assert(pair->name != NULL);
	assert(pair->value != NULL);

	if (strcmp(pair->name, "playlist") != 0 ||
	    !mpd_verify_local_uri(pair->value)) {
		errno = EINVAL;
		return NULL;
	}

	return mpd_playlist_new(pair->value);
}

bool
mpd_playlist_feed(struct mpd_playlist *playlist, const struct mpd_pair *pair)
{
	assert(pair != NULL);
	assert(pair->name != NULL);
	assert(pair->value != NULL);

	if (strcmp(pair->name, "playlist") == 0)
		return false;

	if (strcmp(pair->name, "Last-Modified") == 0)
		playlist->last_modified =
			iso8601_datetime_parse(pair->value);

	return true;
}
