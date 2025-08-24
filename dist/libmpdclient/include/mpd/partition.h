// SPDX-License-Identifier: BSD-2-Clause
// Copyright The Music Player Daemon Project

/*! \file
 * \brief MPD client library
 *
 * Do not include this header directly.  Use mpd/client.h instead.
 */

#ifndef MPD_PARTITION_H
#define MPD_PARTITION_H

#include "recv.h"
#include "compiler.h"

#include <stdbool.h>

struct mpd_pair;
/**
 * \struct mpd_partition
 */
struct mpd_partition;
struct mpd_connection;

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Creates a new partition object from the pair received from MPD server.
 *
 * @param pair a #mpd_pair received from MPD (name must be "partition")
 * @return the new #mpd_partition object, or NULL on error (out of
 * memory, or pair name is not "partition")
 *
 * @since libmpdclient 2.17
 */
mpd_malloc
struct mpd_partition *
mpd_partition_new(const struct mpd_pair *pair);

/**
 * Frees a #mpd_partition object.
 *
 * @since libmpdclient 2.17
 */
void
mpd_partition_free(struct mpd_partition *partition);

/**
 * Returns the partition name.
 *
 * @since libmpdclient 2.17
 */
mpd_pure
const char *
mpd_partition_get_name(const struct mpd_partition *partition);

/**
 * Creates a new partition.
 * A partition is one frontend of a multi-player MPD process: it has separate
 * queue, player and outputs. A client is assigned to one partition at a time.
 *
 * @param connection the connection to MPD
 * @param partition the partition name
 * @return true on success
 *
 * @since libmpdclient 2.17
 */
bool
mpd_send_newpartition(struct mpd_connection *connection, const char *partition);

/**
 * Shortcut for mpd_send_newpartition() and mpd_response_finish().
 *
 * @param connection the connection to MPD
 * @param partition the partition name
 * @return true on success
 *
 * @since libmpdclient 2.17
 */
bool
mpd_run_newpartition(struct mpd_connection *connection, const char *partition);

/**
 * Delete a partition.
 *
 * @param connection the connection to MPD
 * @param partition the partition name
 * @return true on success
 *
 * @since libmpdclient 2.18, MPD 0.22
 */
bool
mpd_send_delete_partition(struct mpd_connection *connection,
			  const char *partition);

/**
 * Shortcut for mpd_send_delete_partition() and mpd_response_finish().
 *
 * @param connection the connection to MPD
 * @param partition the partition name
 * @return true on success
 *
 * @since libmpdclient 2.18, MPD 0.22
 */
bool
mpd_run_delete_partition(struct mpd_connection *connection,
			 const char *partition);

/**
 * Switch the client to a different partition.
 *
 * @param connection the connection to MPD
 * @param partition the partition name
 * @return true on success
 *
 * @since libmpdclient 2.17
 */
bool
mpd_send_switch_partition(struct mpd_connection *connection,
			  const char *partition);

/**
 * Shortcut for mpd_send_switch_partition() and mpd_response_finish().
 *
 * @param connection the connection to MPD
 * @param partition the partition name
 * @return true on success
 *
 * @since libmpdclient 2.17
 */
bool
mpd_run_switch_partition(struct mpd_connection *connection,
			 const char *partition);

/**
 * Sends the "listpartitions" command: request the list of partitions.
 * Call mpd_recv_partition() repeatedly to read the response.
 *
 * @param connection the connection to MPD
 * @return true on success
 *
 * @since libmpdclient 2.17
 */
bool
mpd_send_listpartitions(struct mpd_connection *connection);

/**
 * Receives the next partition name.  Call this in a loop after
 * mpd_send_listpartitions().
 *
 * Free the return value with mpd_return_pair().
 *
 * @param connection a #mpd_connection object
 * @returns a "partition" pair, or NULL on error or if the end of the
 * response is reached
 *
 * @since libmpdclient 2.17
 */
mpd_malloc
static inline struct mpd_pair *
mpd_recv_partition_pair(struct mpd_connection *connection)
{
	return mpd_recv_pair_named(connection, "partition");
}

/**
 * Reads the next #mpd_partition from the MPD response.  Free the
 * return value with mpd_partition_free().
 *
 * @return a mpd_partition object on success, NULL on error or
 * end-of-response
 *
 * @since libmpdclient 2.18
 */
mpd_malloc
struct mpd_partition *
mpd_recv_partition(struct mpd_connection *connection);

#ifdef __cplusplus
}
#endif

#endif
