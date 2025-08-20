// SPDX-License-Identifier: BSD-3-Clause
// Copyright The Music Player Daemon Project

#ifndef LIBMPDCLIENT_STRINGNORMALIZATION_H
#define LIBMPDCLIENT_STRINGNORMALIZATION_H

#include "recv.h"
#include "compiler.h"

#include <stdbool.h>

/**
 * @since libmpdclient 2.24
 */
enum mpd_stringnormalization_option
{
	/**
	 * Special value returned by mpd_stringnormalization_parse() when an
	 * unknown name was passed.
	 */
	MPD_STRINGNORMALIZATION_UNKNOWN = -1,

	MPD_STRINGNORMALIZATION_STRIP_DIACRITICS,

	/* IMPORTANT: the ordering above must be
	   retained, or else the libmpdclient ABI breaks */

	MPD_STRINGNORMALIZATION_COUNT
};

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Looks up the name of the specified stringnormalization option.
 *
 * @return the name, or NULL if the tag type is not valid
 */
const char *
mpd_stringnormalization_name(enum mpd_stringnormalization_option option);

/**
 * Parses a stringnormalization option name, and returns its #mpd_protocol_feature value.
 *
 * @return a #mpd_protocol_feature value, or MPD_FEATURE_UNKNOWN if the name was
 * not recognized
 */
enum mpd_stringnormalization_option
mpd_stringnormalization_name_parse(const char *name);

/**
 * Requests a list of enabled stringnormalization options.
 * Use mpd_recv_stringnormalization_pair() to obtain the list of
 * "option" pairs.
 *
 * @param connection the connection to MPD
 * @return true on success, false on error
 *
 * @since libmpdclient 2.24, MPD 0.25
 */
bool
mpd_send_list_stringnormalization(struct mpd_connection *connection);

/**
 * Requests a list of available protocol features.
 * Use mpd_recv_stringnormalization_pair() to obtain the list of
 * "stringnormalization option" pairs.
 *
 * @param connection the connection to MPD
 * @return true on success, false on error
 *
 * @since libmpdclient 2.24, MPD 0.25
 */
bool
mpd_send_list_stringnormalization_available(struct mpd_connection *connection);

/**
 * Receives the next stringnormalization option.  Call this in a loop after
 * mpd_send_list_stringnormalization().
 *
 * Free the return value with mpd_return_pair().
 *
 * @param connection a #mpd_connection
 * @returns a "stringnormalization option" pair, or NULL on error or if the end of the
 * response is reached
 *
 * @since libmpdclient 2.24, MPD 0.25
 */
mpd_malloc
static inline struct mpd_pair *
mpd_recv_stringnormalization_pair(struct mpd_connection *connection)
{
	return mpd_recv_pair_named(connection, "stringnormalization");
}

/**
 * Disables one or more stringnormalization option from the list of stringnormalization options.
 *
 * @param connection the connection to MPD
 * @param options an array of stringnormalization options to disable
 * @param n the number of protocol features in the array
 * @return true on success, false on error
 *
 * @since libmpdclient 2.24, MPD 0.25
 */
bool
mpd_send_disable_stringnormalization(struct mpd_connection *connection,
				     const enum mpd_stringnormalization_option *options, unsigned n);

/**
 * Shortcut for mpd_send_disable_stringnormalization() and mpd_response_finish().
 *
 * @since libmpdclient 2.24, MPD 0.25
 */
bool
mpd_run_disable_stringnormalization(struct mpd_connection *connection,
				    const enum mpd_stringnormalization_option *options, unsigned n);

/**
 * Re-enable one or more stringnormalization options from the list of stringnormalization options
 * for this client.
 *
 * @param connection the connection to MPD
 * @param options an array of stringnormalization options to enable
 * @param n the number of protocol features in the array
 * @return true on success, false on error
 *
 * @since libmpdclient 2.24, MPD 0.25
 */
bool
mpd_send_enable_stringnormalization(struct mpd_connection *connection,
				    const enum mpd_stringnormalization_option *options, unsigned n);

/**
 * Shortcut for mpd_send_enable_stringnormalization() and mpd_response_finish().
 *
 * @since libmpdclient 2.24, MPD 0.25
 */
bool
mpd_run_enable_stringnormalization(struct mpd_connection *connection,
				   const enum mpd_stringnormalization_option *options, unsigned n);

/**
 * Clear the list of enabled stringnormalization options for this client.
 *
 * @param connection the connection to MPD
 * @return true on success, false on error
 *
 * @since libmpdclient 2.24, MPD 0.25
 */
bool
mpd_send_clear_stringnormalization(struct mpd_connection *connection);

/**
 * Shortcut for mpd_send_clear_stringnormalization() and mpd_response_finish().
 *
 * @since libmpdclient 2.24, MPD 0.25
 */
bool
mpd_run_clear_stringnormalization(struct mpd_connection *connection);

/**
 * Enable all available stringnormalization options for this client.
 *
 * @param connection the connection to MPD
 * @return true on success, false on error
 *
 * @since libmpdclient 2.24, MPD 0.25
 */
bool
mpd_send_all_stringnormalization(struct mpd_connection *connection);

/**
 * Shortcut for mpd_send_all_stringnormalization() and mpd_response_finish().
 *
 * @since libmpdclient 2.24, MPD 0.25
 */
bool
mpd_run_all_stringnormalization(struct mpd_connection *connection);

#ifdef __cplusplus
}
#endif

#endif
