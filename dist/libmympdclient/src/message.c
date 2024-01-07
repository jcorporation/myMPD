// SPDX-License-Identifier: BSD-2-Clause
// Copyright The Music Player Daemon Project

#include <mpd/message.h>
#include <mpd/pair.h>

#include <assert.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

struct mpd_message {
	char *channel;

	char *text;
};

struct mpd_message *
mpd_message_begin(const struct mpd_pair *pair)
{
	struct mpd_message *output;

	assert(pair != NULL);

	if (strcmp(pair->name, "channel") != 0)
		return NULL;

	output = malloc(sizeof(*output));
	if (output == NULL)
		return NULL;

	output->channel = strdup(pair->value);
	output->text = NULL;

	return output;
}

bool
mpd_message_feed(struct mpd_message *output, const struct mpd_pair *pair)
{
	if (strcmp(pair->name, "channel") == 0)
		return false;

	if (strcmp(pair->name, "message") == 0) {
		free(output->text);
		output->text = strdup(pair->value);
	}

	return true;
}

void
mpd_message_free(struct mpd_message *message)
{
	assert(message != NULL);

	free(message->channel);
	free(message->text);
	free(message);
}

const char *
mpd_message_get_channel(const struct mpd_message *message)
{
	assert(message != NULL);

	return message->channel;
}

const char *
mpd_message_get_text(const struct mpd_message *message)
{
	assert(message != NULL);

	return message->text;
}
