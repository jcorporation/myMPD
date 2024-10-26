// SPDX-License-Identifier: BSD-2-Clause
// Copyright The Music Player Daemon Project

#include <mpd/capabilities.h>
#include <mpd/send.h>
#include <mpd/recv.h>
#include <mpd/response.h>
#include "internal.h"

#include <assert.h>
#include <stddef.h>
#include <string.h>

bool
mpd_send_allowed_commands(struct mpd_connection *connection)
{
	return mpd_send_command(connection, "commands", NULL);
}

bool
mpd_send_disallowed_commands(struct mpd_connection *connection)
{
	return mpd_send_command(connection, "notcommands", NULL);
}

bool
mpd_send_list_url_schemes(struct mpd_connection *connection)
{
	return mpd_send_command(connection, "urlhandlers", NULL);
}

bool
mpd_send_list_tag_types(struct mpd_connection *connection)
{
	return mpd_send_command(connection, "tagtypes", NULL);
}

bool
mpd_send_list_tag_types_available(struct mpd_connection *connection)
{
	return mpd_send_command(connection, "tagtypes", "available", NULL);
}

static bool
mpd_send_tag_types_v(struct mpd_connection *connection,
		     const char *sub_command,
		     const enum mpd_tag_type *types, unsigned n)
{
	assert(connection != NULL);
	assert(types != NULL);
	assert(n > 0);

	if (mpd_error_is_defined(&connection->error))
		return false;

	char buffer[1024] = "tagtypes ";
	strcat(buffer, sub_command);
	size_t length = strlen(buffer);

	for (unsigned i = 0; i < n; ++i) {
		const char *t = mpd_tag_name(types[i]);
		assert(t != NULL);
		size_t t_length = strlen(t);

		if (length + 1 + t_length + 1 > sizeof(buffer)) {
			mpd_error_code(&connection->error, MPD_ERROR_ARGUMENT);
			mpd_error_message(&connection->error,
					  "Tag list is too long");
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
mpd_send_disable_tag_types(struct mpd_connection *connection,
			   const enum mpd_tag_type *types, unsigned n)
{
	return mpd_send_tag_types_v(connection, "disable", types, n);
}

bool
mpd_run_disable_tag_types(struct mpd_connection *connection,
			  const enum mpd_tag_type *types, unsigned n)
{
	return mpd_send_disable_tag_types(connection, types, n) &&
		mpd_response_finish(connection);
}

bool
mpd_send_enable_tag_types(struct mpd_connection *connection,
			  const enum mpd_tag_type *types, unsigned n)
{
	return mpd_send_tag_types_v(connection, "enable", types, n);
}

bool
mpd_run_enable_tag_types(struct mpd_connection *connection,
			 const enum mpd_tag_type *types, unsigned n)
{
	return mpd_send_enable_tag_types(connection, types, n) &&
		mpd_response_finish(connection);
}

bool
mpd_send_clear_tag_types(struct mpd_connection *connection)
{
	return mpd_send_command(connection, "tagtypes", "clear", NULL);
}

bool
mpd_run_clear_tag_types(struct mpd_connection *connection)
{
	return mpd_send_clear_tag_types(connection) &&
		mpd_response_finish(connection);
}

bool
mpd_send_all_tag_types(struct mpd_connection *connection)
{
	return mpd_send_command(connection, "tagtypes", "all", NULL);
}

bool
mpd_run_all_tag_types(struct mpd_connection *connection)
{
	return mpd_send_all_tag_types(connection) &&
		mpd_response_finish(connection);
}

bool
mpd_send_list_protocol_features(struct mpd_connection *connection)
{
	return mpd_send_command(connection, "protocol", NULL);
}

bool
mpd_send_list_protocol_features_available(struct mpd_connection *connection)
{
	return mpd_send_command(connection, "protocol", "available", NULL);
}

static bool
mpd_send_protocol_features_v(struct mpd_connection *connection,
			     const char *sub_command,
			     const enum mpd_protocol_feature *features, unsigned n)
{
	assert(connection != NULL);
	assert(features != NULL);
	assert(n > 0);

	if (mpd_error_is_defined(&connection->error))
		return false;

	char buffer[1024] = "protocol ";
	strcat(buffer, sub_command);
	size_t length = strlen(buffer);

	for (unsigned i = 0; i < n; ++i) {
		const char *t = mpd_feature_name(features[i]);
		assert(t != NULL);
		size_t t_length = strlen(t);

		if (length + 1 + t_length + 1 > sizeof(buffer)) {
			mpd_error_code(&connection->error, MPD_ERROR_ARGUMENT);
			mpd_error_message(&connection->error,
					  "Protocol feature list is too long");
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
mpd_send_disable_protocol_features(struct mpd_connection *connection,
			   const enum mpd_protocol_feature *features, unsigned n)
{
	return mpd_send_protocol_features_v(connection, "disable", features, n);
}

bool
mpd_run_disable_protocol_features(struct mpd_connection *connection,
				  const enum mpd_protocol_feature *features, unsigned n)
{
	return mpd_send_disable_protocol_features(connection, features, n) &&
		mpd_response_finish(connection);
}

bool
mpd_send_enable_protocol_features(struct mpd_connection *connection,
				  const enum mpd_protocol_feature *features, unsigned n)
{
	return mpd_send_protocol_features_v(connection, "enable", features, n);
}

bool
mpd_run_enable_protocol_features(struct mpd_connection *connection,
				 const enum mpd_protocol_feature *features, unsigned n)
{
	return mpd_send_enable_protocol_features(connection, features, n) &&
		mpd_response_finish(connection);
}

bool
mpd_send_clear_protocol_features(struct mpd_connection *connection)
{
	return mpd_send_command(connection, "protocol", "clear", NULL);
}

bool
mpd_run_clear_protocol_features(struct mpd_connection *connection)
{
	return mpd_send_clear_protocol_features(connection) &&
		mpd_response_finish(connection);
}

bool
mpd_send_all_protocol_features(struct mpd_connection *connection)
{
	return mpd_send_command(connection, "protocol", "all", NULL);
}

bool
mpd_run_all_protocol_features(struct mpd_connection *connection)
{
	return mpd_send_all_protocol_features(connection) &&
		mpd_response_finish(connection);
}
