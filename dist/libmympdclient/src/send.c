// SPDX-License-Identifier: BSD-2-Clause
// Copyright The Music Player Daemon Project

#include <mpd/send.h>

#include "isend.h"
#include "internal.h"
#include "sync.h"

#include <stdarg.h>
#include <limits.h>
#include <stdio.h>

/* (bits+1)/3 (plus the sign character) */
enum {
	INTLEN = (sizeof(int) * CHAR_BIT + 1) / 3 + 1,
	LONGLONGLEN = (sizeof(long long) * CHAR_BIT + 1) / 3 + 1,
	FLOATLEN = LONGLONGLEN + 8,
};

static void
format_range(char *buffer, size_t size, unsigned start, unsigned end)
{
	if (end == UINT_MAX)
		/* the special value -1 means "open end" */
		snprintf(buffer, size, "%u:", start);
	else
		snprintf(buffer, size, "%u:%u", start, end);
}

static void
format_frange(char *buffer, size_t size, float start, float end)
{
	/* the special value 0.0 means "open range" */
	if (end >= 0)
		snprintf(buffer, size, "%1.3f:%1.3f", (double)start, (double)end);
	else
		snprintf(buffer, size, "%1.3f:", (double)start);
}

/**
 * Checks whether it is possible to send a command now.
 */
static bool
send_check(struct mpd_connection *connection)
{
	assert(connection != NULL);

	if (mpd_error_is_defined(&connection->error))
		return false;

	if (connection->receiving) {
		mpd_error_code(&connection->error, MPD_ERROR_STATE);
		mpd_error_message(&connection->error,
				  "Cannot send a new command while "
				  "receiving another response");
		return false;
	}

	return true;
}

bool
mpd_send_command(struct mpd_connection *connection, const char *command, ...)
{
	va_list ap;
	bool success;

	if (!send_check(connection))
		return false;

	va_start(ap, command);

	success = mpd_sync_send_command_v(connection->async,
					  mpd_connection_timeout(connection),
					  command, ap);

	va_end(ap);

	if (!success) {
		mpd_connection_sync_error(connection);
		return false;
	}

	if (!connection->sending_command_list) {
		/* the caller might expect that we have flushed the
		   output buffer when this function returns */
		if (!mpd_flush(connection))
			return false;

		connection->receiving = true;
	} else if (connection->sending_command_list_ok)
		++connection->command_list_remaining;

	return true;
}

bool
mpd_send_command2(struct mpd_connection *connection, const char *command)
{
	bool success;

	if (!send_check(connection))
		return false;

	success = mpd_sync_send_command(connection->async,
					mpd_connection_timeout(connection),
					command, NULL);
	if (!success) {
		mpd_connection_sync_error(connection);
		return false;
	}

	return true;
}

bool
mpd_send_int_command(struct mpd_connection *connection, const char *command,
		     int arg)
{
	char arg_string[INTLEN];

	snprintf(arg_string, sizeof(arg_string), "%i", arg);
	return mpd_send_command(connection, command, arg_string, NULL);
}

bool
mpd_send_int2_command(struct mpd_connection *connection, const char *command,
		      int arg1, int arg2)
{
	char arg1_string[INTLEN], arg2_string[INTLEN];

	snprintf(arg1_string, sizeof(arg1_string), "%i", arg1);
	snprintf(arg2_string, sizeof(arg2_string), "%i", arg2);
	return mpd_send_command(connection, command,
				arg1_string, arg2_string, NULL);
}

bool
mpd_send_int3_command(struct mpd_connection *connection, const char *command,
		      int arg1, int arg2, int arg3)
{
	char arg1_string[INTLEN], arg2_string[INTLEN], arg3_string[INTLEN];

	snprintf(arg1_string, sizeof(arg1_string), "%i", arg1);
	snprintf(arg2_string, sizeof(arg2_string), "%i", arg2);
	snprintf(arg3_string, sizeof(arg3_string), "%i", arg3);
	return mpd_send_command(connection, command,
				arg1_string, arg2_string, arg3_string, NULL);
}

bool
mpd_send_float_command(struct mpd_connection *connection, const char *command,
		       float arg)
{
	char arg_string[FLOATLEN];

	snprintf(arg_string, sizeof(arg_string), "%f", (double)arg);
	return mpd_send_command(connection, command, arg_string, NULL);
}

bool
mpd_send_u_command(struct mpd_connection *connection, const char *command,
		     unsigned arg1)
{
	char arg1_string[INTLEN];

	snprintf(arg1_string, sizeof(arg1_string), "%u", arg1);
	return mpd_send_command(connection, command,
				arg1_string, NULL);
}

bool
mpd_send_u2_command(struct mpd_connection *connection, const char *command,
		     unsigned arg1, unsigned arg2)
{
	char arg1_string[INTLEN];
	char arg2_string[INTLEN];

	snprintf(arg1_string, sizeof(arg1_string), "%u", arg1);
	snprintf(arg2_string, sizeof(arg2_string), "%u", arg2);
	return mpd_send_command(connection, command,
				arg1_string, arg2_string,NULL);
}

bool
mpd_send_u_f_command(struct mpd_connection *connection, const char *command,
		     unsigned arg1, float arg2)
{
	char arg1_string[INTLEN], arg2_string[FLOATLEN];

	snprintf(arg1_string, sizeof(arg1_string), "%u", arg1);
	snprintf(arg2_string, sizeof(arg2_string), "%.3f", (double)arg2);
	return mpd_send_command(connection, command,
				arg1_string, arg2_string, NULL);
}

bool
mpd_send_u_s_command(struct mpd_connection *connection, const char *command,
		     unsigned arg1, const char *arg2)
{
	char arg1_string[INTLEN];

	snprintf(arg1_string, sizeof(arg1_string), "%i", arg1);
	return mpd_send_command(connection, command,
				arg1_string, arg2, NULL);
}

bool
mpd_send_u_s_s_command(struct mpd_connection *connection, const char *command,
		       unsigned arg1, const char *arg2, const char *arg3)
{
	char arg1_string[INTLEN];

	snprintf(arg1_string, sizeof(arg1_string), "%i", arg1);
	return mpd_send_command(connection, command,
				arg1_string, arg2, arg3, NULL);
}

bool
mpd_send_s_s_command(struct mpd_connection *connection, const char *command,
		       const char *arg1, const char *arg2)
{
	return mpd_send_command(connection, command,
				arg1, arg2, NULL);
}

bool
mpd_send_s_u_command(struct mpd_connection *connection, const char *command,
		     const char *arg1, unsigned arg2)
{
	char arg2_string[INTLEN];

	snprintf(arg2_string, sizeof(arg2_string), "%u", arg2);
	return mpd_send_command(connection, command,
				arg1, arg2_string, NULL);
}

bool
mpd_send_s_s_u_command(struct mpd_connection *connection, const char *command,
		     const char *arg1, const char *arg2, unsigned arg3)
{
	char arg3_string[INTLEN];

	snprintf(arg3_string, sizeof(arg3_string), "%u", arg3);
	return mpd_send_command(connection, command,
				arg1, arg2, arg3_string, NULL);
}

bool
mpd_send_s_s_s_s_u_command(struct mpd_connection *connection, const char *command,
		           const char *arg1, const char *arg2, const char *arg3,
			   const char *arg4, unsigned arg5)
{
	char arg5_string[INTLEN];

	snprintf(arg5_string, sizeof(arg5_string), "%u", arg5);
	return mpd_send_command(connection, command,
				arg1, arg2, arg3, arg4, arg5_string, NULL);
}

bool
mpd_send_range_command(struct mpd_connection *connection, const char *command,
                       unsigned arg1, unsigned arg2)
{
	char arg_string[INTLEN*2+1];

	format_range(arg_string, sizeof(arg_string), arg1, arg2);
	return mpd_send_command(connection, command, arg_string, NULL);
}

bool
mpd_send_s_range_command(struct mpd_connection *connection,
			 const char *command, const char *arg1,
			 unsigned start, unsigned end)
{
	char range_string[INTLEN * 2 + 1];

	format_range(range_string, sizeof(range_string), start, end);
	return mpd_send_command(connection, command,
				arg1, range_string, NULL);
}

bool
mpd_send_s_range_to_command(struct mpd_connection *connection,
			 const char *command, const char *arg1,
			 unsigned start, unsigned end, char *to)
{
	char range_string[INTLEN * 2 + 1];

	format_range(range_string, sizeof(range_string), start, end);
	return mpd_send_command(connection, command,
				arg1, range_string, to, NULL);
}

bool
mpd_send_s_range_to_u_command(struct mpd_connection *connection,
			 const char *command, const char *arg1,
			 unsigned start, unsigned end, unsigned to)
{
	char arg2_string[INTLEN * 2 + 1], arg3_string[INTLEN + 1];

	format_range(arg2_string, sizeof(arg2_string), start, end);
	snprintf(arg3_string, sizeof(arg3_string), "%u", to);
	return mpd_send_command(connection, command,
				arg1, arg2_string, arg3_string, NULL);
}

bool
mpd_send_i_range_command(struct mpd_connection *connection,
			 const char *command, int arg1,
			 unsigned start, unsigned end)
{
	char arg1_string[INTLEN + 1], arg2_string[INTLEN * 2 + 1];

	snprintf(arg1_string, sizeof(arg1_string), "%i", arg1);
	format_range(arg2_string, sizeof(arg2_string), start, end);
	return mpd_send_command(connection, command,
				arg1_string, arg2_string, NULL);
}

bool
mpd_send_u_range_command(struct mpd_connection *connection,
			 const char *command, unsigned arg1,
			 unsigned start, unsigned end)
{
	char arg1_string[INTLEN + 1], arg2_string[INTLEN * 2 + 1];

	snprintf(arg1_string, sizeof(arg1_string), "%u", arg1);
	format_range(arg2_string, sizeof(arg2_string), start, end);
	return mpd_send_command(connection, command,
				arg1_string, arg2_string, NULL);
}

bool
mpd_send_range_u_command(struct mpd_connection *connection,
			 const char *command,
			 unsigned start, unsigned end, unsigned arg2)
{
	char arg1_string[INTLEN*2+1], arg2_string[INTLEN];

	format_range(arg1_string, sizeof(arg1_string), start, end);
	snprintf(arg2_string, sizeof(arg2_string), "%i", arg2);
	return mpd_send_command(connection, command,
				arg1_string, arg2_string, NULL);
}

bool
mpd_send_range_to_command(struct mpd_connection *connection,
			 const char *command,
			 unsigned start, unsigned end, const char *to)
{
	char arg1_string[INTLEN*2+1];

	format_range(arg1_string, sizeof(arg1_string), start, end);
	return mpd_send_command(connection, command,
				arg1_string, to, NULL);
}

bool
mpd_send_u_frange_command(struct mpd_connection *connection,
			 const char *command, unsigned arg1,
			 float start, float end)
{
	/* <start>:<end> */
	char arg1_string[INTLEN + 1];
	char range_string[FLOATLEN * 2 + 1 + 1];

	snprintf(arg1_string, sizeof(arg1_string), "%u", arg1);
	format_frange(range_string, sizeof(range_string), start, end);

	return mpd_send_command(connection, command,
				arg1_string, range_string, NULL);
}

bool
mpd_send_ll_command(struct mpd_connection *connection, const char *command,
		    long long arg)
{
	char arg_string[LONGLONGLEN];

#ifdef _WIN32
	snprintf(arg_string, sizeof(arg_string), "%ld", (long)arg);
#else
	snprintf(arg_string, sizeof(arg_string), "%lld", arg);
#endif
	return mpd_send_command(connection, command, arg_string, NULL);
}

bool
mpd_flush(struct mpd_connection *connection)
{
	if (!mpd_sync_flush(connection->async,
			    mpd_connection_timeout(connection))) {
		mpd_connection_sync_error(connection);
		return false;
	}

	return true;
}
