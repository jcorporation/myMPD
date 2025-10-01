// SPDX-License-Identifier: BSD-2-Clause
// Copyright The Music Player Daemon Project

/*! \file
 * \brief MPD client library
 *
 * Do not include this header directly.  Use mpd/client.h instead.
 */

#ifndef LIBMPDCLIENT_SEND_H
#define LIBMPDCLIENT_SEND_H

#include "compiler.h"

#include <stdbool.h>

struct mpd_connection;

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Sends a command with arguments to the MPD server.  The argument
 * list must be terminated with a NULL.
 *
 * @param connection the connection to the MPD server
 * @param command the command to be sent
 * @return true on success
 */
mpd_sentinel
bool
mpd_send_command(struct mpd_connection *connection, const char *command, ...);

#ifdef __cplusplus
}
#endif

#endif
