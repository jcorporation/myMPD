// SPDX-License-Identifier: BSD-3-Clause
// Copyright The Music Player Daemon Project

/*! \file
 * \brief MPD client library
 *
 * Do not include this header directly.  Use mpd/client.h instead.
 */

#ifndef MPD_READPICTURE_H
#define MPD_READPICTURE_H

#include "compiler.h"

#include <stdbool.h>
#include <stddef.h>

struct mpd_connection;

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Sends the "readpicture" command to MPD.  Call mpd_recv_readpicture() to
 * read response lines.
 *
 * @param connection a valid and connected #mpd_connection
 * @param uri the URI of the song
 * @param offset to read from
 * @return true on success
 *
 * @since libmpdclient 2.20, MPD 0.22
 */
bool
mpd_send_readpicture(struct mpd_connection *connection, const char *uri, unsigned offset);

/**
 * Receives the "readpicture" response
 *
 * @param connection a valid and connected #mpd_connection
 * @param buffer an already allocated buffer, the size must be the same or greater than
 * the binary chunk size (default 8192, can be set with binarylimit command)
 * @param buffer_size the size of the allocated buffer
 * @return read size on success, -1 on failure
 *
 * @since libmpdclient 2.20, MPD 0.22
 */
int
mpd_recv_readpicture(struct mpd_connection *connection, void *buffer, size_t buffer_size);

/**
 * Shortcut for mpd_send_readpicture(), mpd_recv_readpicture() and
 * mpd_response_finish().
 *
 * @param connection a valid and connected #mpd_connection
 * @param uri the URI of the song
 * @param offset to read from
 * @param buffer an already allocated buffer, the size must be the same or greater than
 * the binary chunk size (default 8192, can be set with binarylimit command)
 * @param buffer_size the size of the allocated buffer
 * @return read size on success, -1 on failure
 *
 * @since libmpdclient 2.20, MPD 0.22
 */
int
mpd_run_readpicture(struct mpd_connection *connection,
                    const char *uri, unsigned offset,
                    void *buffer, size_t buffer_size);

#ifdef __cplusplus
}
#endif

#endif
