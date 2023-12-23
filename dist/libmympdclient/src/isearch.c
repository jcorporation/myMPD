// SPDX-License-Identifier: BSD-3-Clause
// Copyright The Music Player Daemon Project

#include "isearch.h"

#include <mpd/send.h>

#include "internal.h"

#include <assert.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char *
mpd_sanitize_arg(const char *src)
{
	assert(src != NULL);

	/* instead of counting in that loop above, just
	 * use a bit more memory and half running time
	 */
	char *result = malloc(strlen(src) * 2 + 1);
	if (result == NULL)
		return NULL;

	char *dest = result;
	char ch;
	do {
		ch = *src++;
		if (ch == '"' || ch == '\\')
			*dest++ = '\\';
		*dest++ = ch;
	} while (ch != 0);

	return result;
}

bool
mpd_request_begin(struct mpd_connection *connection)
{
	assert(connection != NULL);

	if (mpd_error_is_defined(&connection->error))
		return false;

	if (connection->request) {
		mpd_error_code(&connection->error, MPD_ERROR_STATE);
		mpd_error_message(&connection->error,
				  "search already in progress");
		return false;
	}

	return true;
}

bool
mpd_request_command(struct mpd_connection *connection, const char *cmd)
{
	connection->request = strdup(cmd);
	if (connection->request == NULL) {
		mpd_error_code(&connection->error, MPD_ERROR_OOM);
		return false;
	}

	return true;
}

char *
mpd_request_prepare_append(struct mpd_connection *connection,
			  size_t add_length)
{
	assert(connection != NULL);

	if (mpd_error_is_defined(&connection->error))
		return NULL;

	if (connection->request == NULL) {
		mpd_error_code(&connection->error, MPD_ERROR_STATE);
		mpd_error_message(&connection->error,
				  "no search in progress");
		return NULL;
	}

	const size_t old_length = strlen(connection->request);
	char *new_request = realloc(connection->request,
				    old_length + add_length + 1);
	if (new_request == NULL) {
		mpd_error_code(&connection->error, MPD_ERROR_OOM);
		return NULL;
	}

	connection->request = new_request;
	return new_request + old_length;
}

bool
mpd_request_add_sort(struct mpd_connection *connection,
		     const char *name, bool descending)
{
	assert(connection != NULL);

	const size_t size = 64;
	char *dest = mpd_request_prepare_append(connection, size);
	if (dest == NULL)
		return false;

	snprintf(dest, size, " sort %s%s",
		 descending ? "-" : "",
		 name);
	return true;
}

bool
mpd_request_add_window(struct mpd_connection *connection,
		       unsigned start, unsigned end)
{
	assert(connection != NULL);
	assert(start <= end);

	const size_t size = 64;
	char *dest = mpd_request_prepare_append(connection, size);
	if (dest == NULL)
		return false;

	if (end == UINT_MAX)
		/* the special value -1 means "open end" */
		snprintf(dest, size, " window %u:", start);
	else
		snprintf(dest, size, " window %u:%u", start, end);
	return true;
}

bool
mpd_request_commit(struct mpd_connection *connection)
{
	assert(connection != NULL);

	if (mpd_error_is_defined(&connection->error)) {
		mpd_request_cancel(connection);
		return false;
	}

	if (connection->request == NULL) {
		mpd_error_code(&connection->error, MPD_ERROR_STATE);
		mpd_error_message(&connection->error,
				  "no search in progress");
		return false;
	}

	bool success = mpd_send_command(connection, connection->request, NULL);
	free(connection->request);
	connection->request = NULL;

	return success;
}

void
mpd_request_cancel(struct mpd_connection *connection)
{
	assert(connection != NULL);

	free(connection->request);
	connection->request = NULL;
}
