// SPDX-License-Identifier: BSD-2-Clause
// Copyright The Music Player Daemon Project

/*! \file
 * \brief MPD client library
 *
 * Password authentication.
 *
 * Do not include this header directly.  Use mpd/client.h instead.
 */

#ifndef MPD_PASSWORD_H
#define MPD_PASSWORD_H

#include <stdbool.h>

struct mpd_connection;

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Sends the password to MPD, to gain more privileges.
 */
bool
mpd_send_password(struct mpd_connection *connection, const char *password);

/**
 * Sends the password to MPD and receives its response.
 *
 * @return true on success, false on failure
 */
bool
mpd_run_password(struct mpd_connection *connection, const char *password);

#ifdef __cplusplus
}
#endif

#endif
