// SPDX-License-Identifier: BSD-2-Clause
// Copyright The Music Player Daemon Project

#include <mpd/sticker.h>
#include <mpd/connection.h>
#include <mpd/send.h>
#include <mpd/recv.h>
#include <mpd/pair.h>
#include <mpd/response.h>
#include "internal.h"
#include "isend.h"
#include "request.h"
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
mpd_send_sticker_inc(struct mpd_connection *connection, const char *type,
		     const char *uri, const char *name, unsigned value)
{
	return mpd_send_s_s_s_s_u_command(connection, "sticker", "inc",
					type, uri, name, value);
}

bool
mpd_run_sticker_inc(struct mpd_connection *connection, const char *type,
		    const char *uri, const char *name, unsigned value)
{
	return mpd_run_check(connection) &&
		mpd_send_sticker_inc(connection, type, uri, name, value) &&
		mpd_response_finish(connection);
}

bool
mpd_send_sticker_dec(struct mpd_connection *connection, const char *type,
		     const char *uri, const char *name, unsigned value)
{
	return mpd_send_s_s_s_s_u_command(connection, "sticker", "inc",
					type, uri, name, value);
}

bool
mpd_run_sticker_dec(struct mpd_connection *connection, const char *type,
		    const char *uri, const char *name, unsigned value)
{
	return mpd_run_check(connection) &&
		mpd_send_sticker_dec(connection, type, uri, name, value) &&
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
mpd_send_stickertypes(struct mpd_connection *connection)
{
	assert(connection != NULL);

	return mpd_send_command(connection, "stickertypes", NULL);
}

bool
mpd_send_stickernamestypes(struct mpd_connection *connection, const char *type)
{
	assert(connection != NULL);

	return mpd_send_command(connection, "stickernamestypes", type, NULL);
}

bool
mpd_sticker_search_begin(struct mpd_connection *connection, const char *type,
			 const char *base_uri, const char *name)
{
	assert(name != NULL);

	if (!mpd_request_begin(connection)) 
		return false;

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

static const char *get_sticker_oper_str(enum mpd_sticker_operator oper) {
	switch(oper) {
	case MPD_STICKER_OP_EQ:          return "=";
	case MPD_STICKER_OP_GT:          return ">";
	case MPD_STICKER_OP_LT:          return "<";
	case MPD_STICKER_OP_EQ_INT:      return "eq";
	case MPD_STICKER_OP_GT_INT:      return "gt";
	case MPD_STICKER_OP_LT_INT:      return "lt";
	case MPD_STICKER_OP_CONTAINS:    return "contains";
	case MPD_STICKER_OP_STARTS_WITH: return "starts_with";
	case MPD_STICKER_OP_UNKOWN:      return NULL;
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

	const char *oper_str = get_sticker_oper_str(oper);
	if (oper_str == NULL)
		return false;

	const size_t size = 1 + strlen(oper_str) + 2 + strlen(arg) + 2;
	char *dest = mpd_request_prepare_append(connection, size);
	if (dest == NULL) {
		free(arg);
		return false;
	}

	snprintf(dest, size, " %s \"%s\"",
		 oper_str,
		 arg);
	free(arg);
	return true;
}

static const char *get_sticker_sort_name(enum mpd_sticker_sort sort) {
	switch(sort) {
	case MPD_STICKER_SORT_URI:       return "uri";
	case MPD_STICKER_SORT_VALUE:     return "value";
	case MPD_STICKER_SORT_VALUE_INT: return "value_int";
	case MPD_STICKER_SORT_UNKOWN:    return NULL;
	}
	return NULL;
}

bool
mpd_sticker_search_add_sort(struct mpd_connection *connection,
			    enum mpd_sticker_sort sort, bool descending)
{
	const char *sort_str = get_sticker_sort_name(sort);
	return mpd_request_add_sort(connection, sort_str, descending);
}

bool
mpd_sticker_search_add_window(struct mpd_connection *connection,
			      unsigned start, unsigned end)
{
	return mpd_request_add_window(connection, start, end);
}

bool
mpd_sticker_search_commit(struct mpd_connection *connection)
{
	return mpd_request_commit(connection);
}

void
mpd_sticker_search_cancel(struct mpd_connection *connection)
{
	mpd_request_cancel(connection);
}
