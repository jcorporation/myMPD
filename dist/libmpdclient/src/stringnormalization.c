// SPDX-License-Identifier: BSD-3-Clause
// Copyright The Music Player Daemon Project

#include <mpd/stringnormalization.h>
#include <mpd/send.h>
#include <mpd/recv.h>
#include <mpd/response.h>
#include "internal.h"

#include <assert.h>
#include <stddef.h>
#include <string.h>

static const char *const mpd_stringnormalization_options[MPD_STRINGNORMALIZATION_COUNT] =
{
	[MPD_STRINGNORMALIZATION_STRIP_DIACRITICS] = "strip_diacritics",
};

const char *
mpd_stringnormalization_name(enum mpd_stringnormalization_option option)
{
	if ((unsigned)option >= MPD_STRINGNORMALIZATION_COUNT)
		return NULL;

	return mpd_stringnormalization_options[option];
}

enum mpd_stringnormalization_option
mpd_stringnormalization_name_parse(const char *name)
{
	assert(name != NULL);

	for (unsigned i = 0; i < MPD_STRINGNORMALIZATION_COUNT; ++i)
		if (strcmp(name, mpd_stringnormalization_options[i]) == 0)
			return (enum mpd_stringnormalization_option)i;

	return MPD_STRINGNORMALIZATION_UNKNOWN;
}

bool
mpd_send_list_stringnormalization(struct mpd_connection *connection)
{
	return mpd_send_command(connection, "stringnormalization", NULL);
}

bool
mpd_send_list_stringnormalization_available(struct mpd_connection *connection)
{
	return mpd_send_command(connection, "stringnormalization", "available", NULL);
}

static bool
mpd_send_stringnormalization_v(struct mpd_connection *connection,
			     const char *sub_command,
			     const enum mpd_stringnormalization_option *options, unsigned n)
{
	assert(connection != NULL);
	assert(options != NULL);
	assert(n > 0);

	if (mpd_error_is_defined(&connection->error))
		return false;

	char buffer[1024] = "stringnormalization ";
	strcat(buffer, sub_command);
	size_t length = strlen(buffer);

	for (unsigned i = 0; i < n; ++i) {
		const char *t = mpd_stringnormalization_name(options[i]);
		assert(t != NULL);
		size_t t_length = strlen(t);

		if (length + 1 + t_length + 1 > sizeof(buffer)) {
			mpd_error_code(&connection->error, MPD_ERROR_ARGUMENT);
			mpd_error_message(&connection->error,
					  "Stringnormalization list is too long");
			return false;
		}

		buffer[length++] = ' ';
		memcpy(buffer + length, t, t_length);
		length += t_length;
	}

	buffer[length] = 0;

	return mpd_send_command(connection, buffer, NULL);
}

bool
mpd_send_disable_stringnormalization(struct mpd_connection *connection,
			             const enum mpd_stringnormalization_option *options, unsigned n)
{
	return mpd_send_stringnormalization_v(connection, "disable", options, n);
}

bool
mpd_run_disable_stringnormalization(struct mpd_connection *connection,
				    const enum mpd_stringnormalization_option *options, unsigned n)
{
	return mpd_send_disable_stringnormalization(connection, options, n) &&
		mpd_response_finish(connection);
}

bool
mpd_send_enable_stringnormalization(struct mpd_connection *connection,
				    const enum mpd_stringnormalization_option *options, unsigned n)
{
	return mpd_send_stringnormalization_v(connection, "enable", options, n);
}

bool
mpd_run_enable_stringnormalization(struct mpd_connection *connection,
				   const enum mpd_stringnormalization_option *options, unsigned n)
{
	return mpd_send_enable_stringnormalization(connection, options, n) &&
		mpd_response_finish(connection);
}

bool
mpd_send_clear_stringnormalization(struct mpd_connection *connection)
{
	return mpd_send_command(connection, "stringnormalization", "clear", NULL);
}

bool
mpd_run_clear_stringnormalization(struct mpd_connection *connection)
{
	return mpd_send_clear_stringnormalization(connection) &&
		mpd_response_finish(connection);
}

bool
mpd_send_all_stringnormalization(struct mpd_connection *connection)
{
	return mpd_send_command(connection, "stringnormalization", "all", NULL);
}

bool
mpd_run_all_stringnormalization(struct mpd_connection *connection)
{
	return mpd_send_all_stringnormalization(connection) &&
		mpd_response_finish(connection);
}
