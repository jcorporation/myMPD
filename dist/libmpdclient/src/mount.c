// SPDX-License-Identifier: BSD-2-Clause
// Copyright The Music Player Daemon Project

#include <mpd/mount.h>
#include <mpd/pair.h>

#include <assert.h>
#include <string.h>
#include <stdlib.h>

struct mpd_mount {
	char *uri;
	char *storage;
};

struct mpd_mount *
mpd_mount_begin(const struct mpd_pair *pair)
{
	assert(pair != NULL);

	if (strcmp(pair->name, "mount") != 0)
		return NULL;

	struct mpd_mount *mount = malloc(sizeof(*mount));
	if (mount == NULL)
		return NULL;

	mount->uri = strdup(pair->value);
	if (mount->uri == NULL) {
		free(mount);
		return NULL;
	}

	mount->storage = NULL;
	return mount;
}

bool
mpd_mount_feed(struct mpd_mount *mount, const struct mpd_pair *pair)
{
	if (strcmp(pair->name, "mount") == 0)
		return false;

	if (strcmp(pair->name, "storage") == 0) {
		free(mount->storage);
		mount->storage = strdup(pair->value);
	}

	return true;
}

void
mpd_mount_free(struct mpd_mount *mount)
{
	assert(mount != NULL);

	free(mount->uri);
	free(mount->storage);
	free(mount);
}

const char *
mpd_mount_get_uri(const struct mpd_mount *mount)
{
	assert(mount != NULL);

	return mount->uri;
}

const char *
mpd_mount_get_storage(const struct mpd_mount *mount)
{
	assert(mount != NULL);

	return mount->storage;
}
