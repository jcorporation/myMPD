/* libmpdclient
   (c) 2003-2019 The Music Player Daemon Project
   This project's homepage is: http://www.musicpd.org

   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions
   are met:

   - Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.

   - Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.

   - Neither the name of the Music Player Daemon nor the names of its
   contributors may be used to endorse or promote products derived from
   this software without specific prior written permission.

   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
   ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
   A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR
   CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
   EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
   PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
   PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
   LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
   NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
   SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
   */

#include "isearch.h"

#include <mpd/send.h>

#include "internal.h"

#include <assert.h>
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
