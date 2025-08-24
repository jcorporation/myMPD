// SPDX-License-Identifier: BSD-3-Clause
// Copyright The Music Player Daemon Project

/*! \file
 * \brief MPD client library
 *
 * Do not include this header directly.  Use mpd/client.h instead.
 */

#ifndef MPD_RESPONSE_H
#define MPD_RESPONSE_H

#include <stdbool.h>

struct mpd_connection;

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Finishes the response and checks if the command was successful.  If
 * there are data pairs left, they are discarded.
 *
 * @return true on success, false on error
 */
bool
mpd_response_finish(struct mpd_connection *connection);

/**
 * Finishes the response of the current list command.  If there are
 * data pairs left, they are discarded.
 *
 * @return true on success, false on error
 */
bool
mpd_response_next(struct mpd_connection *connection);

#ifdef __cplusplus
}
#endif

#endif
