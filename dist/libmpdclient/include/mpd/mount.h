// SPDX-License-Identifier: BSD-2-Clause
// Copyright The Music Player Daemon Project

/*! \file
 * \brief MPD client library
 *
 * Do not include this header directly.  Use mpd/client.h instead.
 */

#ifndef MPD_MOUNT_H
#define MPD_MOUNT_H

#include "compiler.h"

#include <stdbool.h>

struct mpd_connection;
struct mpd_pair;

/**
 * \struct mpd_mount
 *
 * This type represents a mount point on the MPD server.
 */
struct mpd_mount;

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Begins parsing a new #mpd_mount.
 *
 * @param pair the first pair in this mount point
 * @return the new #mpd_mount object, or NULL on error (out of
 * memory, or wrong pair name)
 *
 * @since libmpdclient 2.16
 */
mpd_malloc
struct mpd_mount *
mpd_mount_begin(const struct mpd_pair *pair);

/**
 * Parses the pair, adding its information to the specified
 * #mpd_mount object.
 *
 * @return true if the pair was parsed and added to the mount (or if
 * the pair was not understood and ignored), false if this pair is the
 * beginning of the next mount
 *
 * @since libmpdclient 2.16
 */
bool
mpd_mount_feed(struct mpd_mount *mnt, const struct mpd_pair *pair);

/**
 * Frees a mpd_mount object returned from mpd_recv_mount() or mpd_mount_begin().
 *
 * @since libmpdclient 2.16
 */
void
mpd_mount_free(struct mpd_mount *mount);

/**
 * @return the mount point URI of the specified #mpd_mount object
 *
 * @since libmpdclient 2.16
 */
mpd_pure
const char *
mpd_mount_get_uri(const struct mpd_mount *mnt);

/**
 * @return the mounted storage URI of the specified #mpd_mount object;
 * may be NULL if MPD did not reveal it
 *
 * @since libmpdclient 2.16
 */
mpd_pure
const char *
mpd_mount_get_storage(const struct mpd_mount *mnt);

/**
 * Sends the "listmounts" command to MPD.  Call mpd_recv_mount() to
 * read the response.
 *
 * @param connection a valid and connected mpd_connection.
 * @return true on success
 *
 * @since libmpdclient 2.16, MPD 0.19
 */
bool
mpd_send_list_mounts(struct mpd_connection *connection);

/**
 * Reads the next mpd_mount from the MPD response.  Free the return
 * value with mpd_mount_free().
 *
 * @return a mpd_mount object on success, NULL on error or
 * end-of-response
 *
 * @since libmpdclient 2.16, MPD 0.19
 */
mpd_malloc
struct mpd_mount *
mpd_recv_mount(struct mpd_connection *connection);

/**
 * Sends the "mount" command to MPD.
 *
 * @param connection a valid and connected mpd_connection.
 * @param uri the mount point URI
 * @param storage the mounted storage URI
 * @return true on success
 *
 * @since libmpdclient 2.16, MPD 0.19
 */
bool
mpd_send_mount(struct mpd_connection *connection,
	       const char *uri, const char *storage);

/**
 * Shortcut for mpd_send_mount() and mpd_response_finish().
 *
 * @param connection A valid and connected mpd_connection.
 * @param uri the mount point URI
 * @param storage the mounted storage URI
 * @return true on success
 *
 * @since libmpdclient 2.16, MPD 0.19
 */
bool
mpd_run_mount(struct mpd_connection *connection,
	      const char *uri, const char *storage);

/**
 * Sends the "unmount" command to MPD.
 *
 * @param connection a valid and connected mpd_connection.
 * @param uri the mount point URI
 * @return true on success
 *
 * @since libmpdclient 2.16, MPD 0.19
 */
bool
mpd_send_unmount(struct mpd_connection *connection, const char *uri);

/**
 * Shortcut for mpd_send_unmount() and mpd_response_finish().
 *
 * @param connection A valid and connected mpd_connection.
 * @param uri the mount point URI
 * @return true on success
 *
 * @since libmpdclient 2.16, MPD 0.19
 */
bool
mpd_run_unmount(struct mpd_connection *connection, const char *uri);

#ifdef __cplusplus
}
#endif

#endif
