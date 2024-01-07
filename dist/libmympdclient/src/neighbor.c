// SPDX-License-Identifier: BSD-2-Clause
// Copyright The Music Player Daemon Project

#include <mpd/neighbor.h>
#include <mpd/pair.h>

#include <assert.h>
#include <string.h>
#include <stdlib.h>

struct mpd_neighbor {
	char *uri;
	char *display_name;
};

struct mpd_neighbor *
mpd_neighbor_begin(const struct mpd_pair *pair)
{
	assert(pair != NULL);

	if (strcmp(pair->name, "neighbor") != 0)
		return NULL;

	struct mpd_neighbor *neighbor = malloc(sizeof(*neighbor));
	if (neighbor == NULL)
		return NULL;

	neighbor->uri = strdup(pair->value);
	if (neighbor->uri == NULL) {
		free(neighbor);
		return NULL;
	}

	neighbor->display_name = NULL;
	return neighbor;
}

bool
mpd_neighbor_feed(struct mpd_neighbor *neighbor, const struct mpd_pair *pair)
{
	if (strcmp(pair->name, "neighbor") == 0)
		return false;

	if (strcmp(pair->name, "name") == 0) {
		free(neighbor->display_name);
		neighbor->display_name = strdup(pair->value);
	}

	return true;
}

void
mpd_neighbor_free(struct mpd_neighbor *neighbor)
{
	assert(neighbor != NULL);

	free(neighbor->uri);
	free(neighbor->display_name);
	free(neighbor);
}

const char *
mpd_neighbor_get_uri(const struct mpd_neighbor *neighbor)
{
	assert(neighbor != NULL);

	return neighbor->uri;
}

const char *
mpd_neighbor_get_display_name(const struct mpd_neighbor *neighbor)
{
	assert(neighbor != NULL);

	return neighbor->display_name;
}
