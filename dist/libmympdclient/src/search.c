// SPDX-License-Identifier: BSD-3-Clause
// Copyright The Music Player Daemon Project

#include <mpd/search.h>
#include <mpd/send.h>
#include <mpd/pair.h>
#include <mpd/recv.h>
#include "internal.h"
#include "request.h"
#include "iso8601.h"

#include <assert.h>
#include <limits.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

bool
mpd_search_db_songs(struct mpd_connection *connection, bool exact)
{
	return mpd_request_begin(connection) &&
	       mpd_request_command(connection,
				   exact ? "find" : "search");
}

bool
mpd_search_add_db_songs(struct mpd_connection *connection, bool exact)
{
	return mpd_request_begin(connection) &&
	       mpd_request_command(connection,
			           exact ? "findadd" : "searchadd");
}

bool
mpd_search_queue_songs(struct mpd_connection *connection, bool exact)
{
	return mpd_request_begin(connection) &&
	       mpd_request_command(connection,
			           exact ? "playlistfind" : "playlistsearch");
}

bool
mpd_search_db_tags(struct mpd_connection *connection, enum mpd_tag_type type)
{
	assert(connection != NULL);

	if (!mpd_request_begin(connection)) 
		return false;

	const char *strtype = mpd_tag_name(type);
	if (strtype == NULL) {
		mpd_error_code(&connection->error, MPD_ERROR_ARGUMENT);
		mpd_error_message(&connection->error,
				  "invalid type specified");
		return false;
	}

	const size_t len = 5 + strlen(strtype) + 1;
	connection->request = malloc(len);
	if (connection->request == NULL) {
		mpd_error_code(&connection->error, MPD_ERROR_OOM);
		return false;
	}

	snprintf(connection->request, len, "list %s", strtype);

	return true;
}

bool
mpd_count_db_songs(struct mpd_connection *connection)
{
	assert(connection != NULL);

	return mpd_request_begin(connection) &&
	       mpd_request_command(connection, "count");
}

bool
mpd_searchcount_db_songs(struct mpd_connection *connection)
{
	assert(connection != NULL);

	return mpd_request_begin(connection) &&
	       mpd_request_command(connection, "searchcount");
}

static bool
mpd_search_add_constraint(struct mpd_connection *connection,
			  mpd_unused enum mpd_operator oper,
			  const char *name,
			  const char *value)
{
	assert(connection != NULL);
	assert(name != NULL);
	assert(value != NULL);

	char *arg = mpd_sanitize_arg(value);
	if (arg == NULL) {
		mpd_error_code(&connection->error, MPD_ERROR_OOM);
		return false;
	}

	const size_t add_length = 1 + strlen(name) + 2 + strlen(arg) + 1;

	char *dest = mpd_request_prepare_append(connection, add_length);
	if (dest == NULL) {
		free(arg);
		return false;
	}

	sprintf(dest, " %s \"%s\"", name, arg);

	free(arg);
	return true;
}
bool
mpd_search_add_base_constraint(struct mpd_connection *connection,
			       enum mpd_operator oper,
			       const char *value)
{
	return mpd_search_add_constraint(connection, oper, "base", value);
}

bool
mpd_search_add_uri_constraint(struct mpd_connection *connection,
			      enum mpd_operator oper,
			      const char *value)
{
	return mpd_search_add_constraint(connection, oper, "file", value);
}

bool
mpd_search_add_tag_constraint(struct mpd_connection *connection,
			      enum mpd_operator oper,
			      enum mpd_tag_type type, const char *value)
{
	assert(connection != NULL);
	assert(value != NULL);

	const char *strtype = mpd_tag_name(type);
	if (strtype == NULL) {
		mpd_error_code(&connection->error, MPD_ERROR_ARGUMENT);
		mpd_error_message(&connection->error,
				  "invalid type specified");
		return false;
	}

	return mpd_search_add_constraint(connection, oper, strtype, value);
}

bool
mpd_search_add_any_tag_constraint(struct mpd_connection *connection,
				  enum mpd_operator oper,
				  const char *value)
{
	return mpd_search_add_constraint(connection, oper, "any", value);
}

bool
mpd_search_add_modified_since_constraint(struct mpd_connection *connection,
					 enum mpd_operator oper,
					 time_t value)
{
	char buffer[64];
	if (!iso8601_datetime_format(buffer, sizeof(buffer), value)) {
		mpd_error_code(&connection->error, MPD_ERROR_ARGUMENT);
		mpd_error_message(&connection->error,
				  "failed to format time stamp");
		return false;
	}

	return mpd_search_add_constraint(connection, oper,
					 "modified-since", buffer);
}

bool
mpd_search_add_added_since_constraint(struct mpd_connection *connection,
				      enum mpd_operator oper,
				      time_t value)
{
	char buffer[64];
	if (!iso8601_datetime_format(buffer, sizeof(buffer), value)) {
		mpd_error_code(&connection->error, MPD_ERROR_ARGUMENT);
		mpd_error_message(&connection->error,
				  "failed to format time stamp");
		return false;
	}

	return mpd_search_add_constraint(connection, oper,
					 "added-since", buffer);
}

bool
mpd_search_add_expression(struct mpd_connection *connection,
			  const char *expression)
{
	assert(connection != NULL);
	assert(expression != NULL);

	char *arg = mpd_sanitize_arg(expression);
	if (arg == NULL) {
		mpd_error_code(&connection->error, MPD_ERROR_OOM);
		return false;
	}

	const size_t add_length = 2 + strlen(arg) + 1;

	char *dest = mpd_request_prepare_append(connection, add_length);
	if (dest == NULL) {
		free(arg);
		return false;
	}

	sprintf(dest, " \"%s\"", arg);

	free(arg);
	return true;
}

bool
mpd_search_add_group_tag(struct mpd_connection *connection,
			 enum mpd_tag_type type)
{
	assert(connection != NULL);

	const size_t size = 64;
	char *dest = mpd_request_prepare_append(connection, size);
	if (dest == NULL)
		return false;

	snprintf(dest, size, " group %s", mpd_tag_name(type));
	return true;
}

bool
mpd_search_add_sort_name(struct mpd_connection *connection,
			 const char *name, bool descending)
{
	return mpd_request_add_sort(connection, name, descending);
}

bool
mpd_search_add_sort_tag(struct mpd_connection *connection,
			enum mpd_tag_type type, bool descending)
{
	return mpd_search_add_sort_name(connection,
					mpd_tag_name(type),
					descending);
}

bool
mpd_search_add_window(struct mpd_connection *connection,
		      unsigned start, unsigned end)
{
	return mpd_request_add_window(connection, start, end);
}

bool
mpd_search_add_position(struct mpd_connection *connection,
			unsigned position, enum mpd_position_whence whence)
{
	assert(connection != NULL);

	const size_t size = 64;
	char *dest = mpd_request_prepare_append(connection, size);
	if (dest == NULL)
		return false;

	const char *whence_s = mpd_position_whence_char(whence);

	snprintf(dest, size, " position %s%u", whence_s, position);
	return true;
}

bool
mpd_search_commit(struct mpd_connection *connection)
{
	return mpd_request_commit(connection);
}

void
mpd_search_cancel(struct mpd_connection *connection)
{
	mpd_request_cancel(connection);
}

struct mpd_pair *
mpd_recv_pair_tag(struct mpd_connection *connection, enum mpd_tag_type type)
{
	assert(connection != NULL);

	const char *name = mpd_tag_name(type);
	if (name == NULL)
		return NULL;

	return mpd_recv_pair_named(connection, name);
}

bool
mpd_search_add_db_songs_to_playlist(struct mpd_connection *connection,
				    const char *playlist_name)
{
	assert(connection != NULL);
	assert(playlist_name != NULL);

	if (!mpd_request_begin(connection)) 
		return false;

	char *arg = mpd_sanitize_arg(playlist_name);
	if (arg == NULL) {
		mpd_error_code(&connection->error, MPD_ERROR_OOM);
		return false;
	}

	const size_t len = 13 + strlen(arg) + 2;
	connection->request = malloc(len);
	if (connection->request == NULL) {
		free(arg);
		mpd_error_code(&connection->error, MPD_ERROR_OOM);
		return false;
	}

	snprintf(connection->request, len, "searchaddpl \"%s\" ", arg);

	free(arg);
	return true;
}
