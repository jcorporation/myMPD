// SPDX-License-Identifier: BSD-3-Clause
// Copyright The Music Player Daemon Project

/*! \file
 * \brief MPD client library
 *
 * Do not include this header directly.  Use mpd/client.h instead.
 */

#ifndef MPD_FINGERPRINT_H
#define MPD_FINGERPRINT_H

#include "compiler.h"

#include <stdbool.h>
#include <stddef.h>

struct mpd_connection;
struct mpd_pair;

enum mpd_fingerprint_type {
	MPD_FINGERPRINT_TYPE_UNKNOWN,
	MPD_FINGERPRINT_TYPE_CHROMAPRINT,
};

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Parse a #mpd_pair name to check which fingerprint type it contains.
 */
mpd_pure
enum mpd_fingerprint_type
mpd_parse_fingerprint_type(const char *name);

/**
 * Sends the "getfingerprint" command to MPD.  Call mpd_recv_pair() to
 * read response lines.  Use mpd_parse_fingerprint_type() to check
 * each pair's name; the pair's value then contains the actual
 * fingerprint.
 *
 * @param connection a valid and connected #mpd_connection
 * @param uri the song URI
 * @return true on success
 *
 * @since libmpdclient 2.17, MPD 0.22
 */
bool
mpd_send_getfingerprint(struct mpd_connection *connection, const char *uri);

/**
 * Shortcut for mpd_send_getfingerprint(), mpd_recv_pair_named() and
 * mpd_response_finish().
 *
 * @param connection a valid and connected #mpd_connection
 * @param uri the song URI
 * @param buffer a buffer for the fingerprint string
 * @param buffer_size the size of the buffer (with enough room for a
 * trailing null byte); if the buffer is too small, behavior is
 * undefined; the library may truncate the string or fail
 * @return a pointer to the buffer on success or NULL on error (or if
 * there was no chromaprint in MPD's response)
 *
 * @since libmpdclient 2.17, MPD 0.22
 */
mpd_malloc
const char *
mpd_run_getfingerprint_chromaprint(struct mpd_connection *connection,
				   const char *uri,
				   char *buffer, size_t buffer_size);

#ifdef __cplusplus
}
#endif

#endif
