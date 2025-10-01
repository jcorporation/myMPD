// SPDX-License-Identifier: BSD-3-Clause
// Copyright The Music Player Daemon Project

/*! \file
 * \brief MPD client library
 *
 * Do not include this header directly.  Use mpd/client.h instead.
 */

#ifndef MPD_BINARY_H
#define MPD_BINARY_H

#include <stdbool.h>

struct mpd_connection;

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Sends the "binarylimit" command to MPD.
 *
 * @param connection a valid and connected mpd_connection.
 * @param limit the binary chunk size limit.
 * @return true on success
 *
 * @since libmpdclient 2.20, MPD 0.22.4
 */
bool
mpd_send_binarylimit(struct mpd_connection *connection, unsigned limit);

/**
 * Shortcut for mpd_send_binarylimit() and mpd_response_finish().
 *
 * @param connection A valid and connected mpd_connection.
 * @param limit the binary chunk size limit.
 * @return true on success
 *
 * @since libmpdclient 2.20, MPD 0.22.4
 */
bool
mpd_run_binarylimit(struct mpd_connection *connection, unsigned limit);

#ifdef __cplusplus
}
#endif

#endif
