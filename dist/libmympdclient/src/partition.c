// SPDX-License-Identifier: BSD-2-Clause
// Copyright The Music Player Daemon Project

#include <mpd/pair.h>
#include <mpd/partition.h>

#include <assert.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

struct mpd_partition {
	char *name;
};

struct mpd_partition *
mpd_partition_new(const struct mpd_pair *pair)
{
	assert(pair != NULL);

	if (strcmp(pair->name, "partition") != 0)
		return NULL;

	struct mpd_partition *partition = malloc(sizeof(*partition));
	if (partition == NULL)
		return NULL;

	partition->name = strdup(pair->value);
	if (partition->name == NULL) {
		free(partition);
		return NULL;
	}

	return partition;
}

void
mpd_partition_free(struct mpd_partition *partition)
{
	assert(partition != NULL);

	free(partition->name);
	free(partition);
}

mpd_pure
const char *
mpd_partition_get_name(const struct mpd_partition *partition)
{
	assert(partition != NULL);

	return partition->name;
}
