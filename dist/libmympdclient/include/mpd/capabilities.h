// SPDX-License-Identifier: BSD-2-Clause
// Copyright The Music Player Daemon Project

/*! \file
 * \brief MPD client library
 *
 * Do not include this header directly.  Use mpd/client.h instead.
 */

#ifndef MPD_CAPABILITIES_H
#define MPD_CAPABILITIES_H

#include "recv.h"
#include "compiler.h"
#include "tag.h"

#include <stdbool.h>

struct mpd_connection;

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Requests a list of supported and allowed commands. Use
 * mpd_recv_command_pair() to obtain the list of "command" pairs.
 *
 * @param connection the connection to MPD
 * @return true on success, false on error
 */
bool
mpd_send_allowed_commands(struct mpd_connection *connection);

/**
 * Requests a list of supported commands which are not allowed for
 * this connection.  Use mpd_recv_command_pair() to obtain the list of
 * "command" pairs.
 *
 * @param connection the connection to MPD
 * @return true on success, false on error
 */
bool
mpd_send_disallowed_commands(struct mpd_connection *connection);

/**
 * Receives the next supported command.  Call this in a loop after
 * mpd_send_allowed_commands() or mpd_send_disallowed_commands().
 *
 * Free the return value with mpd_return_pair().
 *
 * @param connection a #mpd_connection
 * @returns a "command" pair, or NULL on error or if the end of the
 * response is reached
 */
mpd_malloc
static inline struct mpd_pair *
mpd_recv_command_pair(struct mpd_connection *connection)
{
	return mpd_recv_pair_named(connection, "command");
}

/**
 * Requests a list of supported URL handlers in the form "scheme://",
 * example: "http://".  Use mpd_recv_url_scheme_pair() to obtain the list
 * of "handler" pairs.
 *
 * @param connection the connection to MPD
 * @return true on success, false on error
 */
bool
mpd_send_list_url_schemes(struct mpd_connection *connection);

/**
 * Receives one line of the mpd_send_url_schemes() response.
 *
 * Free the return value with mpd_return_pair().
 *
 * @param connection a #mpd_connection
 * @returns a "handler" pair, or NULL on error or if the end of the
 * response is reached
 */
mpd_malloc
static inline struct mpd_pair *
mpd_recv_url_scheme_pair(struct mpd_connection *connection)
{
	return mpd_recv_pair_named(connection, "handler");
}

/**
 * Requests a list of supported tag types.  Use mpd_recv_tag_type_pair()
 * to obtain the list of "tagtype" pairs.
 *
 * @param connection the connection to MPD
 * @return true on success, false on error
 */
bool
mpd_send_list_tag_types(struct mpd_connection *connection);

/**
 * Receives the next tag type name.  Call this in a loop after
 * mpd_send_list_tag_types().
 *
 * Free the return value with mpd_return_pair().
 *
 * @param connection a #mpd_connection
 * @returns a "tagtype" pair, or NULL on error or if the end of the
 * response is reached
 */
mpd_malloc
static inline struct mpd_pair *
mpd_recv_tag_type_pair(struct mpd_connection *connection)
{
	return mpd_recv_pair_named(connection, "tagtype");
}

/**
 * Remove one or more tags from the list of tag types the client is
 * interested in.  These will be omitted from responses to this
 * client.
 *
 * @param connection the connection to MPD
 * @param types an array of tag types to disable
 * @param n the number of tag types in the array
 * @return true on success, false on error
 *
 * @since libmpdclient 2.12, MPD 0.21
 */
bool
mpd_send_disable_tag_types(struct mpd_connection *connection,
			   const enum mpd_tag_type *types, unsigned n);

/**
 * Shortcut for mpd_send_disable_tag_types() and mpd_response_finish().
 *
 * @since libmpdclient 2.12, MPD 0.21
 */
bool
mpd_run_disable_tag_types(struct mpd_connection *connection,
			   const enum mpd_tag_type *types, unsigned n);

/**
 * Re-enable one or more tags from the list of tag types for this
 * client.  These will no longer be hidden from responses to this
 * client.
 *
 * @param connection the connection to MPD
 * @param types an array of tag types to enable
 * @param n the number of tag types in the array
 * @return true on success, false on error
 *
 * @since libmpdclient 2.12, MPD 0.21
 */
bool
mpd_send_enable_tag_types(struct mpd_connection *connection,
			  const enum mpd_tag_type *types, unsigned n);

/**
 * Shortcut for mpd_send_enable_tag_types() and mpd_response_finish().
 *
 * @since libmpdclient 2.12, MPD 0.21
 */
bool
mpd_run_enable_tag_types(struct mpd_connection *connection,
			 const enum mpd_tag_type *types, unsigned n);

/**
 * Clear the list of tag types this client is interested in.  This
 * means that MPD will not send any tags to this client.
 *
 * @param connection the connection to MPD
 * @return true on success, false on error
 *
 * @since libmpdclient 2.12, MPD 0.21
 */
bool
mpd_send_clear_tag_types(struct mpd_connection *connection);

/**
 * Shortcut for mpd_send_clear_tag_types() and mpd_response_finish().
 *
 * @since libmpdclient 2.12, MPD 0.21
 */
bool
mpd_run_clear_tag_types(struct mpd_connection *connection);

/**
 * Add all tags to the list of tag types this client is interested in.
 * This means that MPD will send all tags to this client.
 *
 * @param connection the connection to MPD
 * @return true on success, false on error
 *
 * @since libmpdclient 2.19, MPD 0.21
 */
bool
mpd_send_all_tag_types(struct mpd_connection *connection);

/**
 * Shortcut for mpd_send_all_tag_types() and mpd_response_finish().
 *
 * @since libmpdclient 2.19, MPD 0.21
 */
bool
mpd_run_all_tag_types(struct mpd_connection *connection);

#ifdef __cplusplus
}
#endif

#endif
