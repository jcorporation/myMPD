// SPDX-License-Identifier: BSD-2-Clause
// Copyright The Music Player Daemon Project

#ifndef MPD_ISEND_H
#define MPD_ISEND_H

#include <stdbool.h>

struct mpd_connection;

/**
 * Sends a command without arguments to the server, but does not
 * update the "receiving" flag nor the "listOks" counter.  This is
 * used internally by the command_list functions.
 */
bool
mpd_send_command2(struct mpd_connection *connection, const char *command);

bool
mpd_send_int_command(struct mpd_connection *connection, const char *command,
		     int arg);

bool
mpd_send_int2_command(struct mpd_connection *connection, const char *command,
		      int arg1, int arg2);

bool
mpd_send_int3_command(struct mpd_connection *connection, const char *command,
		      int arg1, int arg2, int arg3);

bool
mpd_send_float_command(struct mpd_connection *connection, const char *command,
		       float arg);

bool
mpd_send_u_command(struct mpd_connection *connection, const char *command,
		     unsigned arg1);

bool
mpd_send_u2_command(struct mpd_connection *connection, const char *command,
		     unsigned arg1, unsigned arg2);

bool
mpd_send_u_f_command(struct mpd_connection *connection, const char *command,
		     unsigned arg1, float arg2);

bool
mpd_send_u_s_command(struct mpd_connection *connection, const char *command,
		     unsigned arg1, const char *arg2);

bool
mpd_send_s_s_command(struct mpd_connection *connection, const char *command,
		       const char *arg1, const char *arg2);

bool
mpd_send_u_s_s_command(struct mpd_connection *connection, const char *command,
		       unsigned arg1, const char *arg2, const char *arg3);

bool
mpd_send_s_u_command(struct mpd_connection *connection, const char *command,
		     const char *arg1, unsigned arg2);

bool
mpd_send_s_s_u_command(struct mpd_connection *connection, const char *command,
		     const char *arg1, const char *arg2, unsigned arg3);

bool
mpd_send_range_command(struct mpd_connection *connection, const char *command,
		       unsigned arg1, unsigned arg2);

/**
 * Send command with one string argument followed by a range argument.
 */
bool
mpd_send_s_range_command(struct mpd_connection *connection,
			 const char *command, const char *arg1,
			 unsigned start, unsigned end);

/**
 * Send command with one integer argument followed by a range argument.
 */
bool
mpd_send_i_range_command(struct mpd_connection *connection,
			 const char *command, int arg1,
			 unsigned start, unsigned end);

/**
 * Send command with one integer argument followed by a range argument
 * and a to parameter.
 */
bool
mpd_send_s_range_to_command(struct mpd_connection *connection,
			 const char *command, const char *arg1,
			 unsigned start, unsigned end, char *to);

bool
mpd_send_s_range_to_u_command(struct mpd_connection *connection,
			 const char *command, const char *arg1,
			 unsigned start, unsigned end, unsigned to);

bool
mpd_send_u_range_command(struct mpd_connection *connection,
			 const char *command, unsigned arg1,
			 unsigned start, unsigned end);

/**
 * Send command with a range argument followed by a to parameter.
 */
bool
mpd_send_range_to_command(struct mpd_connection *connection,
			 const char *command,
			 unsigned start, unsigned end, const char *to);

bool
mpd_send_range_u_command(struct mpd_connection *connection,
			 const char *command,
			 unsigned start, unsigned end, unsigned arg2);
bool
mpd_send_u_frange_command(struct mpd_connection *connection,
			  const char *command, unsigned arg1,
			  float start, float end);

bool
mpd_send_ll_command(struct mpd_connection *connection, const char *command,
		    long long arg);

/**
 * Sends all pending data from the output buffer to MPD.
 */
bool
mpd_flush(struct mpd_connection *connection);

#endif
