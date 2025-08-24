// SPDX-License-Identifier: BSD-2-Clause
// Copyright The Music Player Daemon Project

/*! \file
 * \brief MPD client library
 *
 * Library to determine connection settings prior to calling
 * mpd_connection_new().
 *
 * Do not include this header directly.  Use mpd/client.h instead.
 */

#ifndef MPD_SETTINGS_H
#define MPD_SETTINGS_H

#include <stdbool.h>

/**
 * \struct mpd_settings
 *
 * An object which describes configurable connection settings.
 */
struct mpd_settings;

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Creates a new #mpd_settings object.  The values which are not
 * passed by the caller are taken from environment variables.
 *
 * @param host the server's host name, IP address or Unix socket path.
 * An address starting with '@' denotes an "abstract socket".
 * NULL is allowed here, which will connect to the default host
 * (using the MPD_HOST environment variable if present).
 * @param port the TCP port to connect to, 0 for default port (using
 * the MPD_PORT environment variable if present).  If "host" is a Unix
 * socket path, this parameter is ignored.
 * @param timeout_ms the timeout in milliseconds, 0 for the default
 * timeout (the environment variable MPD_TIMEOUT may specify a timeout
 * in seconds)
 * @param reserved reserved for future use, pass NULL
 * @param password the password, or NULL to use the default (MPD_HOST
 * before "@")
 * @return a #mpd_settings object or NULL if out of memory
 *
 * @since libmpdclient 2.4
 */
struct mpd_settings *
mpd_settings_new(const char *host, unsigned port, unsigned timeout_ms,
		 const char *reserved, const char *password);

/**
 * Releases a #mpd_settings object.
 *
 * @since libmpdclient 2.4
 */
void
mpd_settings_free(struct mpd_settings *settings);

/**
 * Returns the host name (without password/port), or NULL if unknown.
 *
 * @since libmpdclient 2.4
 */
const char *
mpd_settings_get_host(const struct mpd_settings *settings);

/**
 * Returns the port number, or 0 if not applicable.
 *
 * @since libmpdclient 2.4
 */
unsigned
mpd_settings_get_port(const struct mpd_settings *settings);

/**
 * Returns the timeout in milliseconds, or 0 if unknown.
 *
 * @since libmpdclient 2.4
 */
unsigned
mpd_settings_get_timeout_ms(const struct mpd_settings *settings);

/**
 * Returns the password, or NULL if none was configured.
 *
 * @since libmpdclient 2.4
 */
const char *
mpd_settings_get_password(const struct mpd_settings *settings);

#ifdef __cplusplus
}
#endif

#endif
