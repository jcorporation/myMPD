// SPDX-License-Identifier: BSD-3-Clause
// Copyright The Music Player Daemon Project

#include <mpd/directory.h>
#include <mpd/pair.h>
#include "uri.h"
#include "iso8601.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

struct mpd_directory {
	/**
	 * The full path of this directory.  It does not begin with a
	 * slash.
	 */
	char *path;

	/**
	 * The POSIX UTC time stamp of the last modification, or 0 if
	 * that is unknown.
	 */
	time_t last_modified;
};

mpd_malloc
static struct mpd_directory *
mpd_directory_new(const char *path)
{
	struct mpd_directory *directory;

	assert(path != NULL);
	assert(mpd_verify_local_uri(path));

	directory = malloc(sizeof(*directory));
	if (directory == NULL)
		/* out of memory */
		return NULL;

	directory->path = strdup(path);
	if (directory->path == NULL) {
		/* out of memory */
		free(directory);
		return NULL;
	}

	directory->last_modified = 0;

	return directory;
}

void mpd_directory_free(struct mpd_directory *directory)
{
	assert(directory != NULL);
	assert(directory->path != NULL);

	free(directory->path);
	free(directory);
}

struct mpd_directory *
mpd_directory_dup(const struct mpd_directory *directory)
{
	assert(directory != NULL);
	assert(directory->path != NULL);

	struct mpd_directory *copy = mpd_directory_new(directory->path);
	copy->last_modified = directory->last_modified;
	return copy;
}

const char *
mpd_directory_get_path(const struct mpd_directory *directory)
{
	assert(directory != NULL);
	assert(directory->path != NULL);

	return directory->path;
}

time_t
mpd_directory_get_last_modified(const struct mpd_directory *directory)
{
	assert(directory != NULL);

	return directory->last_modified;
}

struct mpd_directory *
mpd_directory_begin(const struct mpd_pair *pair)
{
	assert(pair != NULL);
	assert(pair->name != NULL);
	assert(pair->value != NULL);

	if (strcmp(pair->name, "directory") != 0 ||
	    !mpd_verify_local_uri(pair->value)) {
		errno = EINVAL;
		return NULL;
	}

	return mpd_directory_new(pair->value);
}

bool
mpd_directory_feed(struct mpd_directory *directory,
		   const struct mpd_pair *pair)
{
	assert(pair != NULL);
	assert(pair->name != NULL);
	assert(pair->value != NULL);

	if (strcmp(pair->name, "directory") == 0)
		return false;

	if (strcmp(pair->name, "Last-Modified") == 0)
		directory->last_modified =
			iso8601_datetime_parse(pair->value);

	return true;
}
