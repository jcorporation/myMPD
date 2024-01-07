// SPDX-License-Identifier: BSD-3-Clause
// Copyright The Music Player Daemon Project

#include <mpd/entity.h>
#include <mpd/playlist.h>
#include <mpd/recv.h>
#include "internal.h"

#include <stdlib.h>
#include <string.h>
#include <errno.h>

struct mpd_entity {
	/**
	 * The type of this entity.
	 */
	enum mpd_entity_type type;

	/**
	 * This union contains type-safe pointers to the real object.
	 * Check the entity type before attempting to obtain the
	 * object!
	 */
	union {
		/**
		 * Only valid if type==#MPD_ENTITY_TYPE_DIRECTORY.
		 */
		struct mpd_directory *directory;

		/**
		 * Only valid if type==#MPD_ENTITY_TYPE_SONG.
		 */
		struct mpd_song *song;

		/**
		 * Only valid if type==#MPD_ENTITY_TYPE_PLAYLIST.
		 */
		struct mpd_playlist *playlistFile;
	} info;
};

void
mpd_entity_free(struct mpd_entity *entity) {
	assert(entity != NULL);

	switch (entity->type) {
	case MPD_ENTITY_TYPE_UNKNOWN:
		/* nothing to free */
		break;

	case MPD_ENTITY_TYPE_DIRECTORY:
		mpd_directory_free(entity->info.directory);
		break;

	case MPD_ENTITY_TYPE_SONG:
		mpd_song_free(entity->info.song);
		break;

	case MPD_ENTITY_TYPE_PLAYLIST:
		mpd_playlist_free(entity->info.playlistFile);
		break;
	}

	free(entity);
}

enum mpd_entity_type
mpd_entity_get_type(const struct mpd_entity *entity)
{
	assert(entity != NULL);

	return entity->type;
}

const struct mpd_directory *
mpd_entity_get_directory(const struct mpd_entity *entity)
{
	assert(entity != NULL);
	assert(entity->type == MPD_ENTITY_TYPE_DIRECTORY);

	return entity->info.directory;
}

const struct mpd_song *
mpd_entity_get_song(const struct mpd_entity *entity)
{
	assert(entity != NULL);
	assert(entity->type == MPD_ENTITY_TYPE_SONG);

	return entity->info.song;
}

const struct mpd_playlist *
mpd_entity_get_playlist(const struct mpd_entity *entity)
{
	assert(entity != NULL);
	assert(entity->type == MPD_ENTITY_TYPE_PLAYLIST);

	return entity->info.playlistFile;
}

static bool
mpd_entity_feed_first(struct mpd_entity *entity, const struct mpd_pair *pair)
{
	if (strcmp(pair->name, "file") == 0) {
		entity->type = MPD_ENTITY_TYPE_SONG;
		entity->info.song = mpd_song_begin(pair);
		if (entity->info.song == NULL)
			return false;
	} else if (strcmp(pair->name, "directory") == 0) {
		entity->type = MPD_ENTITY_TYPE_DIRECTORY;
		entity->info.directory = mpd_directory_begin(pair);
		if (entity->info.directory == NULL)
			return false;
	} else if (strcmp(pair->name, "playlist") == 0) {
		entity->type = MPD_ENTITY_TYPE_PLAYLIST;
		entity->info.playlistFile = mpd_playlist_begin(pair);
		if (entity->info.playlistFile == NULL)
			return false;
	} else {
		entity->type = MPD_ENTITY_TYPE_UNKNOWN;
	}

	return true;
}

struct mpd_entity *
mpd_entity_begin(const struct mpd_pair *pair)
{
	struct mpd_entity *entity;
	bool success;

	entity = malloc(sizeof(*entity));
	if (entity == NULL)
		/* out of memory */
		return NULL;

	success = mpd_entity_feed_first(entity, pair);
	if (!success) {
		free(entity);
		return NULL;
	}

	return entity;
}

bool
mpd_entity_feed(struct mpd_entity *entity, const struct mpd_pair *pair)
{
	assert(pair != NULL);
	assert(pair->name != NULL);
	assert(pair->value != NULL);

	if (strcmp(pair->name, "file") == 0 ||
	    strcmp(pair->name, "directory") == 0 ||
	    strcmp(pair->name, "playlist") == 0)
		return false;

	switch (entity->type) {
	case MPD_ENTITY_TYPE_UNKNOWN:
		break;

	case MPD_ENTITY_TYPE_DIRECTORY:
		mpd_directory_feed(entity->info.directory, pair);
		break;

	case MPD_ENTITY_TYPE_SONG:
		mpd_song_feed(entity->info.song, pair);
		break;

	case MPD_ENTITY_TYPE_PLAYLIST:
		mpd_playlist_feed(entity->info.playlistFile, pair);
		break;
	}

	return true;
}

struct mpd_entity *
mpd_recv_entity(struct mpd_connection *connection)
{
	struct mpd_pair *pair;
	struct mpd_entity *entity;

	pair = mpd_recv_pair(connection);
	if (pair == NULL)
		return NULL;

	entity = mpd_entity_begin(pair);
	mpd_return_pair(connection, pair);
	if (entity == NULL) {
		mpd_error_entity(&connection->error);
		return NULL;
	}

	while ((pair = mpd_recv_pair(connection)) != NULL &&
	       mpd_entity_feed(entity, pair))
		mpd_return_pair(connection, pair);

	if (mpd_error_is_defined(&connection->error)) {
		mpd_entity_free(entity);
		return NULL;
	}

	/* unread this pair for the next mpd_recv_entity() call */
	mpd_enqueue_pair(connection, pair);

	return entity;
}
