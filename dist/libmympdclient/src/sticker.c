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

#include <mpd/sticker.h>
#include <mpd/connection.h>
#include <mpd/send.h>
#include <mpd/recv.h>
#include <mpd/pair.h>
#include <mpd/response.h>
#include "internal.h"
#include "isearch.h"
#include "run.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

bool
mpd_send_sticker_set(struct mpd_connection *connection, const char *type,
		     const char *uri, const char *name, const char *value)
{
	return mpd_send_command(connection, "sticker", "set",
				type, uri, name, value, NULL);
}

bool
mpd_run_sticker_set(struct mpd_connection *connection, const char *type,
		    const char *uri, const char *name, const char *value)
{
	return mpd_run_check(connection) &&
		mpd_send_sticker_set(connection, type, uri, name, value) &&
		mpd_response_finish(connection);
}

bool
mpd_send_sticker_delete(struct mpd_connection *connection, const char *type,
			const char *uri, const char *name)
{
	assert(connection != NULL);
	assert(type != NULL);
	assert(uri != NULL);
	assert(name != NULL);

	return mpd_send_command(connection, "sticker", "delete",
				type, uri, name, NULL);
}

bool
mpd_run_sticker_delete(struct mpd_connection *connection, const char *type,
		       const char *uri, const char *name)
{
	return mpd_run_check(connection) &&
		mpd_send_sticker_delete(connection, type, uri, name) &&
		mpd_response_finish(connection);
}

bool
mpd_send_sticker_get(struct mpd_connection *connection, const char *type,
		     const char *uri, const char *name)
{
	assert(connection != NULL);
	assert(type != NULL);
	assert(uri != NULL);
	assert(name != NULL);

	return mpd_send_command(connection, "sticker", "get",
				type, uri, name, NULL);
}

bool
mpd_send_sticker_list(struct mpd_connection *connection, const char *type, const char *uri)
{
	assert(connection != NULL);
	assert(type != NULL);
	assert(uri != NULL);

	return mpd_send_command(connection, "sticker", "list",
				type, uri, NULL);
}

bool
mpd_send_sticker_find(struct mpd_connection *connection, const char *type,
		      const char *base_uri, const char *name)
{
	assert(connection != NULL);
	assert(type != NULL);
	assert(name != NULL);

	if (base_uri == NULL)
		base_uri = "";

	return mpd_send_command(connection, "sticker", "find",
				type, base_uri, name, NULL);
}

const char *
mpd_parse_sticker(const char *input, size_t *name_length_r)
{
	const char *eq;

	eq = strchr(input, '=');
	if (eq == NULL || eq == input)
		return NULL;

	*name_length_r = eq - input;
	return eq + 1;
}

struct mpd_pair *
mpd_recv_sticker(struct mpd_connection *connection)
{
	struct mpd_pair *pair;
	char *eq;

	pair = mpd_recv_pair_named(connection, "sticker");
	if (pair == NULL)
		return NULL;

	pair->name = pair->value;

	eq = strchr(pair->value, '=');
	if (eq != NULL) {
		/* we shouldn't modify a const string, but in this
		   case, we know that this points to the writable
		   input buffer */
		*eq = 0;
		pair->value = eq + 1;
	} else
		/* malformed response?  what to do now?  pretend
		   nothing has happened... */
		pair->value = "";

	return pair;
}

void
mpd_return_sticker(struct mpd_connection *connection, struct mpd_pair *pair)
{
	mpd_return_pair(connection, pair);
}

bool
mpd_send_stickernames(struct mpd_connection *connection)
{
	assert(connection != NULL);

	return mpd_send_command(connection, "stickernames", NULL);
}

bool
mpd_sticker_search_begin(struct mpd_connection *connection, const char *type,
			 const char *base_uri, const char *name)
{
	assert(connection != NULL);
	assert(name != NULL);

	if (mpd_error_is_defined(&connection->error))
		return false;

	if (connection->request) {
		mpd_error_code(&connection->error, MPD_ERROR_STATE);
		mpd_error_message(&connection->error,
				  "search already in progress");
		return false;
	}

	if (base_uri == NULL)
		base_uri = "";

	char *arg_base_uri = mpd_sanitize_arg(base_uri);
	if (arg_base_uri == NULL) {
		mpd_error_code(&connection->error, MPD_ERROR_OOM);
		return false;
	}

	char *arg_name = mpd_sanitize_arg(name);
	if (arg_name == NULL) {
		mpd_error_code(&connection->error, MPD_ERROR_OOM);
		free(arg_base_uri);
		return false;
	}

	const size_t size = 13 + strlen(type) + 2 + strlen(arg_base_uri) + 3 + strlen(arg_name) + 2;
	connection->request = malloc(size);
	if (connection->request == NULL) {
		mpd_error_code(&connection->error, MPD_ERROR_OOM);
		free(arg_base_uri);
		free(arg_name);
		return false;
	}

	snprintf(connection->request, size, "sticker find %s \"%s\" \"%s\"",
		type, arg_base_uri, arg_name);

	free(arg_base_uri);
	free(arg_name);
	return true;
}

static const char *get_sticker_opper_str(enum mpd_sticker_operator oper) {
	switch(oper) {
	case MPD_STICKER_OP_EQ: return "=";
	case MPD_STICKER_OP_GT: return ">";
	case MPD_STICKER_OP_LT: return "<";
	case MPD_STICKER_OP_UNKOWN: return NULL;
	}
	return NULL;
}

bool
mpd_sticker_search_add_value_constraint(struct mpd_connection *connection,
					enum mpd_sticker_operator oper,
					const char *value)
{
	assert(connection != NULL);
	assert(value != NULL);

	char *arg = mpd_sanitize_arg(value);
	if (arg == NULL) {
		mpd_error_code(&connection->error, MPD_ERROR_OOM);
		return false;
	}

	const size_t size = 4 + strlen(arg) + 2;
	char *dest = mpd_search_prepare_append(connection, size);
	if (dest == NULL) {
		free(arg);
		return false;
	}

	const char *oper_str = get_sticker_opper_str(oper);
	if (oper_str == NULL)
		return false;

	snprintf(dest, size, " %s \"%s\"",
		 oper_str,
		 arg);
	free(arg);
	return true;
}

bool
mpd_sticker_search_add_sort(struct mpd_connection *connection,
			 const char *name, bool descending)
{
	assert(connection != NULL);

	const size_t size = 64;
	char *dest = mpd_search_prepare_append(connection, size);
	if (dest == NULL)
		return false;

	snprintf(dest, size, " sort %s%s",
		 descending ? "-" : "",
		 name);
	return true;
}

bool
mpd_sticker_search_add_window(struct mpd_connection *connection,
			      unsigned start, unsigned end)
{
	assert(connection != NULL);
	assert(start <= end);

	const size_t size = 64;
	char *dest = mpd_search_prepare_append(connection, size);
	if (dest == NULL)
		return false;

	snprintf(dest, size, " window %u:%u", start, end);
	return true;
}

bool
mpd_sticker_search_commit(struct mpd_connection *connection)
{
	assert(connection != NULL);

	if (mpd_error_is_defined(&connection->error)) {
		mpd_sticker_search_cancel(connection);
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
mpd_sticker_search_cancel(struct mpd_connection *connection)
{
	assert(connection != NULL);

	free(connection->request);
	connection->request = NULL;
}
