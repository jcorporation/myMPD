// SPDX-License-Identifier: BSD-2-Clause
// Copyright The Music Player Daemon Project

#include <mpd/message.h>
#include <mpd/send.h>
#include <mpd/response.h>
#include "internal.h"
#include "run.h"

#include <assert.h>
#include <stddef.h>

bool
mpd_send_subscribe(struct mpd_connection *connection, const char *channel)
{
	return mpd_send_command(connection, "subscribe", channel, NULL);
}

bool
mpd_run_subscribe(struct mpd_connection *connection, const char *channel)
{
	return mpd_run_check(connection) &&
		mpd_send_subscribe(connection, channel) &&
		mpd_response_finish(connection);
}

bool
mpd_send_unsubscribe(struct mpd_connection *connection, const char *channel)
{
	return mpd_send_command(connection, "unsubscribe", channel, NULL);
}

bool
mpd_run_unsubscribe(struct mpd_connection *connection, const char *channel)
{
	return mpd_run_check(connection) &&
		mpd_send_unsubscribe(connection, channel) &&
		mpd_response_finish(connection);
}

bool
mpd_send_send_message(struct mpd_connection *connection,
		      const char *channel, const char *text)
{
	return mpd_send_command(connection, "sendmessage", channel, text,
				NULL);
}

bool
mpd_run_send_message(struct mpd_connection *connection,
		     const char *channel, const char *text)
{
	return mpd_run_check(connection) &&
		mpd_send_send_message(connection, channel, text) &&
		mpd_response_finish(connection);
}

bool
mpd_send_read_messages(struct mpd_connection *connection)
{
	return mpd_send_command(connection, "readmessages", NULL);
}

struct mpd_message *
mpd_recv_message(struct mpd_connection *connection)
{
	struct mpd_message *message;
	struct mpd_pair *pair;

	pair = mpd_recv_pair_named(connection, "channel");
	if (pair == NULL)
		return NULL;

	message = mpd_message_begin(pair);
	mpd_return_pair(connection, pair);
	if (message == NULL) {
		mpd_error_code(&connection->error, MPD_ERROR_OOM);
		return NULL;
	}

	while ((pair = mpd_recv_pair(connection)) != NULL &&
	       mpd_message_feed(message, pair))
		mpd_return_pair(connection, pair);

	if (mpd_error_is_defined(&connection->error)) {
		assert(pair == NULL);

		mpd_message_free(message);
		return NULL;
	}

	mpd_enqueue_pair(connection, pair);

	if (mpd_message_get_text(message) == NULL) {
		mpd_error_code(&connection->error, MPD_ERROR_MALFORMED);
		mpd_error_message(&connection->error,
				  "No 'message' line received");
		mpd_message_free(message);
		return NULL;
	}

	return message;
}

bool
mpd_send_channels(struct mpd_connection *connection)
{
	return mpd_send_command(connection, "channels", NULL);
}
