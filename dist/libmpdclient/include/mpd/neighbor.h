// SPDX-License-Identifier: BSD-2-Clause
// Copyright The Music Player Daemon Project

/*! \file
 * \brief MPD client library
 *
 * Do not include this header directly.  Use mpd/client.h instead.
 */

#ifndef MPD_NEIGHBOR_H
#define MPD_NEIGHBOR_H

#include "compiler.h"

#include <stdbool.h>

struct mpd_connection;
struct mpd_pair;

/**
 * \struct mpd_neighbor
 *
 * This type represents a neighbor (accessible file servers on the local net)
 * on the MPD server.
 */
struct mpd_neighbor;

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Begins parsing a new #mpd_neighbor.
 *
 * @param pair the first pair in this mount point
 * @return the new #mpd_neighbor object, or NULL on error (out of
 * memory, or wrong pair name)
 *
 * @since libmpdclient 2.17
 */
mpd_malloc
struct mpd_neighbor *
mpd_neighbor_begin(const struct mpd_pair *pair);

/**
 * Parses the pair, adding its information to the specified
 * #mpd_neighbor object.
 *
 * @return true if the pair was parsed and added to the mpd_neighbor (or if
 * the pair was not understood and ignored), false if this pair is the
 * beginning of the next neighbor
 *
 * @since libmpdclient 2.17
 */
bool
mpd_neighbor_feed(struct mpd_neighbor *neighbor, const struct mpd_pair *pair);

/**
 * Frees a #mpd_neighbor object returned from mpd_recv_neighbor() or 
 * mpd_neighbor_begin().
 *
 * @since libmpdclient 2.17
 */
void
mpd_neighbor_free(struct mpd_neighbor *neighbor);

/**
 * @return the neighbor mount point URI of the specified #mpd_neighbor object
 *
 * @since libmpdclient 2.17
 */
mpd_pure
const char *
mpd_neighbor_get_uri(const struct mpd_neighbor *neighbor);

/**
 * @return the display name of the specified #mpd_neighbor object;
 *
 * @since libmpdclient 2.17
 */
mpd_pure
const char *
mpd_neighbor_get_display_name(const struct mpd_neighbor *neighbor);

/**
 * Sends the "listneighbors" command to MPD: queries a list of "neighbors"
 * (e.g. accessible file servers on the local net). Call mpd_recv_neighbor() to
 * read the response.
 *
 * @param connection a valid and connected mpd_connection.
 * @return true on success
 *
 * @since libmpdclient 2.17, MPD 0.19
 */
bool
mpd_send_list_neighbors(struct mpd_connection *connection);

/**
 * Reads the next mpd_neighbor from the MPD response.  Free the return
 * value with mpd_neighbor_free().
 *
 * @return a mpd_neighbor object on success, NULL on error or
 * end-of-response
 *
 * @since libmpdclient 2.17, MPD 0.19
 */
mpd_malloc
struct mpd_neighbor *
mpd_recv_neighbor(struct mpd_connection *connection);

#ifdef __cplusplus
}
#endif

#endif
