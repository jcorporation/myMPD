// SPDX-License-Identifier: BSD-2-Clause
// Copyright The Music Player Daemon Project

/*! \file
 * \brief MPD client library
 *
 * Functions for manipulating MPD's mixer controls.
 *
 * Do not include this header directly.  Use mpd/client.h instead.
 */

#ifndef MPD_MIXER_H
#define MPD_MIXER_H

#include <stdbool.h>

struct mpd_connection;

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Sets the volume of all output devices.
 *
 * @param connection the connection to MPD
 * @param volume the volume, an integer between 0 and 100
 * @return true on success, false on error
 */
bool
mpd_send_set_volume(struct mpd_connection *connection, unsigned volume);

/**
 * Shortcut for mpd_send_set_volume() and mpd_response_finish().
 *
 * @param connection the connection to MPD
 * @param volume the volume, an integer between 0 and 100
 * @return true on success, false on error
 */
bool
mpd_run_set_volume(struct mpd_connection *connection, unsigned volume);

/**
 * Changes the volume of all output devices.
 *
 * This function uses a deprecated feature of MPD, call
 * mpd_send_set_volume() instead.
 *
 * @param connection the connection to MPD
 * @param relative_volume the relative volume, an integer between -100 and 100
 * @return true on success, false on error
 *
 * @since libmpdclient 2.9
 */
bool
mpd_send_change_volume(struct mpd_connection *connection, int relative_volume);

/**
 * Shortcut for mpd_send_change_volume() and mpd_response_finish().
 *
 * This function uses a deprecated feature of MPD, call
 * mpd_run_set_volume() instead.
 *
 * @param connection the connection to MPD
 * @param relative_volume the relative volume, an integer between -100 and 100
 * @return true on success, false on error
 *
 * @since libmpdclient 2.9
 */
bool
mpd_run_change_volume(struct mpd_connection *connection, int relative_volume);

/**
 * Sends the "getvol" command to MPD.  Call mpd_recv_pair() to
 * read the response line.
 *
 * @param connection a valid and connected #mpd_connection
 * @return true on success
 *
 * @since libmpdclient 2.20, MPD 0.23
 */
bool
mpd_send_get_volume(struct mpd_connection *connection);

/**
 * Shortcut for mpd_send_get_volume(), mpd_recv_pair_named() and
 * mpd_response_finish().
 *
 * @param connection a valid and connected #mpd_connection
 * @return volume on success or -1 on error
 *
 * @since libmpdclient 2.20, MPD 0.23
 */
int
mpd_run_get_volume(struct mpd_connection *connection);

#ifdef __cplusplus
}
#endif

#endif
