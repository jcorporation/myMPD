// SPDX-License-Identifier: BSD-2-Clause
// Copyright The Music Player Daemon Project

#ifndef LIBMPDCLIENT_PARSER_H
#define LIBMPDCLIENT_PARSER_H

#include "protocol.h"
#include "compiler.h"

#include <stdbool.h>

enum mpd_parser_result {
	/**
	 * Response line was not understood.
	 */
	MPD_PARSER_MALFORMED,

	/**
	 * MPD has returned "OK" or "list_OK" (check with
	 * mpd_parser_is_discrete()).
	 */
	MPD_PARSER_SUCCESS,

	/**
	 * MPD has returned "ACK" with an error code.  Call
	 * mpd_parser_get_server_error() to get the error code.
	 */
	MPD_PARSER_ERROR,

	/**
	 * MPD has returned a name-value pair.  Call
	 * mpd_parser_get_name() and mpd_parser_get_value().
	 */
	MPD_PARSER_PAIR,
};

/**
 * \struct mpd_parser
 *
 * This opaque object is a low-level parser for the MPD protocol.  You
 * feed it with input lines, and it provides parsed representations.
 */
struct mpd_parser;

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Allocates a new mpd_parser object.  Returns NULL on error (out of
 * memory).
 */
mpd_malloc
struct mpd_parser *
mpd_parser_new(void);

/**
 * Frees a mpd_parser object.
 */
void
mpd_parser_free(struct mpd_parser *parser);

/**
 * Feeds a line (without the trailing newline character) received from
 * MPD / mpd_async_recv_line() into the parser.
 *
 * Note that the line parameter is writable, because the parser will
 * modify it.  The functions mpd_parser_get_name() and
 * mpd_parser_get_value() will return pointers inside this buffer.
 * This means that after passing the line to this function, you must
 * not modify or free it, until the name and value pointers are not
 * used anymore.
 *
 * @param parser the #mpd_parser object
 * @param line a line received from the MPD server
 * @return a result code indicating the type of line, or error
 */
enum mpd_parser_result
mpd_parser_feed(struct mpd_parser *parser, char *line);

/**
 * Call this when mpd_parser_feed() has returned #MPD_PARSER_SUCCESS
 * to find out whether this is an "OK" (false) or a "list_OK" (true)
 * response.
 *
 * @param parser the #mpd_parser object
 */
mpd_pure
bool
mpd_parser_is_discrete(const struct mpd_parser *parser);

/**
 * Call this when mpd_parser_feed() has returned #MPD_PARSER_ERROR to
 * obtain the reason for the error.
 *
 * @param parser the #mpd_parser object
 */
mpd_pure
enum mpd_server_error
mpd_parser_get_server_error(const struct mpd_parser *parser);

/**
 * On #MPD_PARSER_ERROR, this returns the number of the list command
 * which failed.  Don't call this outside of a command list.
 *
 * @param parser the #mpd_parser object
 */
mpd_pure
unsigned
mpd_parser_get_at(const struct mpd_parser *parser);

/**
 * On #MPD_PARSER_ERROR, this returns the human readable error message
 * returned by MPD (UTF-8).
 *
 * This returns a pointer into the line buffer passed to
 * mpd_parser_feed().  It is valid as long as the buffer is not
 * freed/modified.
 *
 * @param parser the #mpd_parser object
 */
mpd_pure
const char *
mpd_parser_get_message(const struct mpd_parser *parser);

/**
 * On #MPD_PARSER_PAIR, this returns the name.
 *
 * This returns a pointer into the line buffer passed to
 * mpd_parser_feed().  It is valid as long as the buffer is not
 * freed/modified.
 *
 * @param parser the #mpd_parser object
 */
mpd_pure
const char *
mpd_parser_get_name(const struct mpd_parser *parser);

/**
 * On #MPD_PARSER_PAIR, this returns the value.
 *
 * This returns a pointer into the line buffer passed to
 * mpd_parser_feed().  It is valid as long as the buffer is not
 * freed/modified.
 *
 * @param parser the #mpd_parser object
 */
mpd_pure
const char *
mpd_parser_get_value(const struct mpd_parser *parser);

#ifdef __cplusplus
}
#endif

#endif
