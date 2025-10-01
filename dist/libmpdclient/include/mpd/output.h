// SPDX-License-Identifier: BSD-3-Clause
// Copyright The Music Player Daemon Project

/*! \file
 * \brief MPD client library
 *
 * Do not include this header directly.  Use mpd/client.h instead.
 */

#ifndef MPD_OUTPUT_H
#define MPD_OUTPUT_H

#include "compiler.h"

#include <stdbool.h>

struct mpd_connection;
struct mpd_pair;

/**
 * \struct mpd_output
 *
 * This type represents an audio output device on the MPD server.
 */
struct mpd_output;

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Begins parsing a new #mpd_output.
 *
 * @param pair the first pair in this output (name is "outputid")
 * @return the new #mpd_output object, or NULL on error (out of
 * memory, or wrong pair name)
 */
mpd_malloc
struct mpd_output *
mpd_output_begin(const struct mpd_pair *pair);

/**
 * Parses the pair, adding its information to the specified
 * #mpd_output object.
 *
 * @return true if the pair was parsed and added to the output (or if
 * the pair was not understood and ignored), false if this pair is the
 * beginning of the next output
 */
bool
mpd_output_feed(struct mpd_output *output, const struct mpd_pair *pair);

/**
 * Frees a mpd_output object returned from mpd_recv_output() or
 * mpd_output_begin().
 */
void
mpd_output_free(struct mpd_output *output);

/**
 * @return the id of the specified #mpd_output object
 */
mpd_pure
unsigned
mpd_output_get_id(const struct mpd_output *output);

/**
 * @return the configured name of the specified #mpd_output object
 */
mpd_pure
const char *
mpd_output_get_name(const struct mpd_output *output);

/**
 * @return the plugin of the specified #mpd_output object, or NULL if
 * none was specified by the server
 *
 * @since libmpdclient 2.14, MPD 0.21
 */
mpd_pure
const char *
mpd_output_get_plugin(const struct mpd_output *output);

/**
 * @return true if this output is enabled
 */
mpd_pure
bool
mpd_output_get_enabled(const struct mpd_output *output);

/**
 * Find an attribute with the given name and return its value.
 *
 * @return the attribute value or NULL if there is no such attributes
 *
 * @since libmpdclient 2.16, MPD 0.21
 */
mpd_pure
const char *
mpd_output_get_attribute(const struct mpd_output *output, const char *name);

/**
 * Obtains the first attribute for this output.  This rewinds the
 * current attribute pointer to the start.  Call
 * mpd_output_next_attribute() to obtain more attributes.
 *
 * @return a pointer to the first attribute or NULL if there are no
 * attributes
 *
 * @since libmpdclient 2.14, MPD 0.21
 */
const struct mpd_pair *
mpd_output_first_attribute(struct mpd_output *output);

/**
 * Obtains the next attribute for this output.  Call this function
 * repeatedly until it returns NULL to get a full list of attributes.
 *
 * @return a pointer to the next attribute or NULL if there are no
 * more attributes
 *
 * @since libmpdclient 2.14, MPD 0.21
 */
const struct mpd_pair *
mpd_output_next_attribute(struct mpd_output *output);

/**
 * Sends the "outputs" command to MPD: fetch information about all outputs.
 * Call mpd_recv_output() to read the response.
 *
 * @param connection A valid and connected mpd_connection.
 * @return true on success
 */
bool
mpd_send_outputs(struct mpd_connection *connection);

/**
 * Reads the next mpd_output from the MPD response.  Free the return
 * value with mpd_output_free().
 *
 * @return a mpd_output object on success, NULL on error or
 * end-of-response
 */
mpd_malloc
struct mpd_output *
mpd_recv_output(struct mpd_connection *connection);

/**
 * Sends the "enableoutput" command to MPD.
 *
 * @param connection A valid and connected mpd_connection.
 * @param output_id an identifier for the output device (see
 * mpd_recv_output())
 * @return true on success
 */
bool
mpd_send_enable_output(struct mpd_connection *connection, unsigned output_id);

/**
 * Shortcut for mpd_send_enable_output() and mpd_response_finish().
 *
 * @param connection A valid and connected mpd_connection.
 * @param output_id an identifier for the output device (see
 * mpd_recv_output())
 * @return true on success
 */
bool
mpd_run_enable_output(struct mpd_connection *connection, unsigned output_id);

/**
 * Sends the "disableoutput" command to MPD.
 *
 * @param connection A valid and connected mpd_connection.
 * @param output_id an identifier for the output device (see
 * mpd_recv_output())
 * @return true on success
 */
bool
mpd_send_disable_output(struct mpd_connection *connection, unsigned output_id);

/**
 * Shortcut for mpd_send_disable_output() and mpd_response_finish().
 *
 * @param connection A valid and connected mpd_connection.
 * @param output_id an identifier for the output device (see
 * mpd_recv_output())
 * @return true on success
 */
bool
mpd_run_disable_output(struct mpd_connection *connection, unsigned output_id);

/**
 * Sends the "toggleoutput" command to MPD.
 *
 * @param connection a valid and connected mpd_connection.
 * @param output_id an identifier for the output device (see
 * mpd_recv_output())
 * @return true on success
 *
 * @since libmpdclient 2.9
 */
bool
mpd_send_toggle_output(struct mpd_connection *connection, unsigned output_id);

/**
 * Shortcut for mpd_send_toggle_output() and mpd_response_finish().
 *
 * @param connection a valid and connected mpd_connection.
 * @param output_id an identifier for the output device (see
 * mpd_recv_output())
 * @return true on success
 *
 * @since libmpdclient 2.9
 */
bool
mpd_run_toggle_output(struct mpd_connection *connection, unsigned output_id);

/**
 * Sends the "outputset" command to MPD: set a runtime attribute for the
 * specified output_id.
 *
 * @param connection a valid and connected mpd_connection
 * @param output_id an identifier for the output device (see
 * mpd_recv_output())
 * @param attribute_name the attribute name
 * @param attribute_value the attribute value
 * @return true on success
 *
 * @since libmpdclient 2.14, MPD 0.21
 */
bool
mpd_send_output_set(struct mpd_connection *connection, unsigned output_id,
		    const char *attribute_name, const char *attribute_value);

/**
 * Shortcut for mpd_send_output_set() and mpd_response_finish().
 *
 * @since libmpdclient 2.14, MPD 0.21
 */
bool
mpd_run_output_set(struct mpd_connection *connection, unsigned output_id,
		   const char *attribute_name, const char *attribute_value);

/**
 * Move an output to the current partition.
 *
 * @param connection the connection to MPD
 * @param output_name the name of the output to be moved
 * @return true on success
 *
 * @since libmpdclient 2.18, MPD 0.22.
 */
bool
mpd_send_move_output(struct mpd_connection *connection,
		     const char *output_name);

/**
 * Shortcut for mpd_send_move_output() and mpd_response_finish().
 *
 * @param connection the connection to MPD
 * @param output_name the name of the output to be moved
 * @return true on success
 *
 * @since libmpdclient 2.18, MPD 0.22.
 */
bool
mpd_run_move_output(struct mpd_connection *connection,
		    const char *output_name);

#ifdef __cplusplus
}
#endif

#endif
