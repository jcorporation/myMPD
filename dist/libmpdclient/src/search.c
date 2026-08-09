// SPDX-License-Identifier: BSD-3-Clause
// Copyright The Music Player Daemon Project

#include <mpd/search.h>
#include <mpd/send.h>
#include <mpd/pair.h>
#include <mpd/recv.h>
#include "internal.h"
#include "request.h"
#include "quote.h"
#include "iso8601.h"
#include "check_tag.h"

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

	const char *strtype = mpd_check_tag_name(type, &connection->error);
	if (strtype == NULL)
		return false;

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

	const size_t name_length = strlen(name);

	/* worst-case allocation */
	const size_t size = 1 + name_length + 2 + strlen(value) * 2 + 2;
	char *const start = mpd_request_prepare_append(connection, size);
	if (start == NULL)
		return false;

	char *p = start;
	char *const end = start + size - 1;

	*p++ = ' ';

	memcpy(p, name, name_length);
	p += name_length;

	*p++ = ' ';

	p = quote(p, end, value);
	if (p == NULL) {
		/* undo this partial append: the null terminator of
		   the previous request was overwritten above */
		*start = '\0';

		mpd_error_code(&connection->error, MPD_ERROR_ARGUMENT);
		mpd_error_message(&connection->error, "bad string");
		return false;
	}

	*p = '\0';
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

	const char *strtype = mpd_check_tag_name(type, &connection->error);
	if (strtype == NULL)
		return false;

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

	/* worst-case allocation */
	const size_t size = 2 + strlen(expression) * 2 + 2;
	char *const start = mpd_request_prepare_append(connection, size);
	if (start == NULL)
		return false;

	char *p = start;
	char *const end = start + size - 1;

	*p++ = ' ';

	p = quote(p, end, expression);
	if (p == NULL) {
		/* undo this partial append: the null terminator of
		   the previous request was overwritten above */
		*start = '\0';

		mpd_error_code(&connection->error, MPD_ERROR_ARGUMENT);
		mpd_error_message(&connection->error, "bad string");
		return false;
	}

	*p = '\0';
	return true;
}

bool
mpd_search_add_group_tag(struct mpd_connection *connection,
			 enum mpd_tag_type type)
{
	assert(connection != NULL);

	const char *tag_name = mpd_check_tag_name(type, &connection->error);
	if (tag_name == NULL)
		return false;

	const size_t size = 64;
	char *dest = mpd_request_prepare_append(connection, size);
	if (dest == NULL)
		return false;

	snprintf(dest, size, " group %s", tag_name);
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
	const char *tag_name = mpd_check_tag_name(type, &connection->error);
	if (tag_name == NULL)
		return false;

	return mpd_search_add_sort_name(connection, tag_name, descending);
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

	const char *name = mpd_check_tag_name(type, &connection->error);
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

	static const char *const prefix = "searchaddpl ";
	const size_t prefix_length = strlen(prefix);

	/* worst-case allocation */
	const size_t size = prefix_length + 1 + strlen(playlist_name) * 2 + 3;
	char *p = connection->request = malloc(size);
	if (connection->request == NULL) {
		mpd_error_code(&connection->error, MPD_ERROR_OOM);
		return false;
	}

	char *const end = p + size - 1;

	memcpy(p, prefix, prefix_length);
	p += prefix_length;

	p = quote(p, end, playlist_name);
	if (p == NULL) {
		mpd_request_cancel(connection);
		mpd_error_code(&connection->error, MPD_ERROR_ARGUMENT);
		mpd_error_message(&connection->error, "bad string");
		return false;
	}

	*p++ = ' ';
	*p = '\0';
	return true;
}
