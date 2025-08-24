// SPDX-License-Identifier: BSD-3-Clause
// Copyright The Music Player Daemon Project

/*! \file
 * \brief MPD client library
 *
 * Do not include this header directly.  Use mpd/client.h instead.
 */

#ifndef MPD_DIRECTORY_H
#define MPD_DIRECTORY_H

#include "compiler.h"

#include <stdbool.h>
#include <time.h>

struct mpd_pair;
struct mpd_connection;

/**
 * \struct mpd_directory
 *
 * An opaque directory object.  This is a container for more songs,
 * directories or playlists.
 */
struct mpd_directory;

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Duplicates a #mpd_directory object.
 *
 * @return the new object, or NULL on out of memory
 */
mpd_malloc
struct mpd_directory *
mpd_directory_dup(const struct mpd_directory *directory);

/**
 * Free memory allocated by the #mpd_directory object.
 */
void mpd_directory_free(struct mpd_directory *directory);

/**
 * Returns the path of this directory, relative to the MPD music
 * directory.  It does not begin with a slash.
 */
mpd_pure
const char *
mpd_directory_get_path(const struct mpd_directory *directory);

/**
 * @return the POSIX UTC time stamp of the last modification, or 0 if
 * that is unknown
 *
 * @since libmpdclient 2.9
 */
mpd_pure
time_t
mpd_directory_get_last_modified(const struct mpd_directory *directory);

/**
 * Begins parsing a new directory.
 *
 * @param pair the first pair in this directory (name must be "directory")
 * @return the new #mpd_entity object, or NULL on error (out of
 * memory, or pair name is not "directory")
 */
mpd_malloc
struct mpd_directory *
mpd_directory_begin(const struct mpd_pair *pair);

/**
 * Parses the pair, adding its information to the specified
 * #mpd_directory object.
 *
 * @return true if the pair was parsed and added to the directory (or if
 * the pair was not understood and ignored), false if this pair is the
 * beginning of the next directory
 */
bool
mpd_directory_feed(struct mpd_directory *directory,
		   const struct mpd_pair *pair);

/**
 * Receives the next directory from the MPD server.
 *
 * @return a #mpd_directory object, or NULL on error or if the directory list is
 * finished
 */
mpd_malloc
struct mpd_directory *
mpd_recv_directory(struct mpd_connection *connection);

#ifdef __cplusplus
}
#endif

#endif
