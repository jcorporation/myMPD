// SPDX-License-Identifier: BSD-3-Clause
// Copyright The Music Player Daemon Project

#include <mpd/stats.h>
#include <mpd/pair.h>

#include <assert.h>
#include <stdlib.h>
#include <string.h>

struct mpd_stats {
	unsigned number_of_artists;
	unsigned number_of_albums;
	unsigned number_of_songs;
	unsigned long uptime;
	unsigned long db_update_time;
	unsigned long play_time;
	unsigned long db_play_time;
};

struct mpd_stats *
mpd_stats_begin(void)
{
	struct mpd_stats *stats = malloc(sizeof(struct mpd_stats));
	if (stats == NULL)
		return NULL;

	stats->number_of_artists = 0;
	stats->number_of_albums = 0;
	stats->number_of_songs = 0;
	stats->uptime = 0;
	stats->db_update_time = 0;
	stats->play_time = 0;
	stats->db_play_time = 0;

	return stats;
}

void
mpd_stats_feed(struct mpd_stats *stats, const struct mpd_pair *pair)
{
	if (strcmp(pair->name, "artists") == 0)
		stats->number_of_artists = strtoul(pair->value, NULL, 10);
	else if (strcmp(pair->name, "albums") == 0)
		stats->number_of_albums = strtoul(pair->value, NULL, 10);
	else if (strcmp(pair->name, "songs") == 0)
		stats->number_of_songs = strtoul(pair->value, NULL, 10);
	else if (strcmp(pair->name, "uptime") == 0)
		stats->uptime = strtoul(pair->value, NULL, 10);
	else if (strcmp(pair->name, "db_update") == 0)
		stats->db_update_time = strtoul(pair->value, NULL, 10);
	else if (strcmp(pair->name, "playtime") == 0)
		stats->play_time = strtoul(pair->value, NULL, 10);
	else if (strcmp(pair->name, "db_playtime") == 0)
		stats->db_play_time = strtoul(pair->value, NULL, 10);
}

void mpd_stats_free(struct mpd_stats * stats) {
	assert(stats != NULL);

	free(stats);
}

unsigned
mpd_stats_get_number_of_artists(const struct mpd_stats * stats)
{
	assert(stats != NULL);

	return stats->number_of_artists;
}

unsigned
mpd_stats_get_number_of_albums(const struct mpd_stats * stats)
{
	assert(stats != NULL);

	return stats->number_of_albums;
}

unsigned
mpd_stats_get_number_of_songs(const struct mpd_stats * stats)
{
	assert(stats != NULL);

	return stats->number_of_songs;
}

unsigned long mpd_stats_get_uptime(const struct mpd_stats * stats)
{
	assert(stats != NULL);

	return stats->uptime;
}

unsigned long mpd_stats_get_db_update_time(const struct mpd_stats * stats)
{
	assert(stats != NULL);

	return stats->db_update_time;
}

unsigned long mpd_stats_get_play_time(const struct mpd_stats * stats)
{
	assert(stats != NULL);

	return stats->play_time;
}

unsigned long mpd_stats_get_db_play_time(const struct mpd_stats * stats)
{
	assert(stats != NULL);

	return stats->db_play_time;
}
