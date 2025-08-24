// SPDX-License-Identifier: BSD-3-Clause
// Copyright The Music Player Daemon Project

#include <mpd/output.h>
#include <mpd/pair.h>
#include "kvlist.h"

#include <assert.h>
#include <string.h>
#include <stdlib.h>

struct mpd_output {
	unsigned id;
	char *name;
	char *plugin;

	struct mpd_kvlist attributes;

	bool enabled;
};

struct mpd_output *
mpd_output_begin(const struct mpd_pair *pair)
{
	struct mpd_output *output;

	assert(pair != NULL);

	if (strcmp(pair->name, "outputid") != 0)
		return NULL;

	output = malloc(sizeof(*output));
	if (output == NULL)
		return NULL;

	output->id = strtoul(pair->value, NULL, 10);

	output->name = NULL;
	output->plugin = NULL;
	mpd_kvlist_init(&output->attributes);
	output->enabled = false;

	return output;
}

bool
mpd_output_feed(struct mpd_output *output, const struct mpd_pair *pair)
{
	if (strcmp(pair->name, "outputid") == 0)
		return false;

	if (strcmp(pair->name, "outputname") == 0) {
		free(output->name);
		output->name = strdup(pair->value);
	} else if (strcmp(pair->name, "outputenabled") == 0)
		output->enabled = atoi(pair->value) != 0;
	else if (strcmp(pair->name, "plugin") == 0) {
		free(output->plugin);
		output->plugin = strdup(pair->value);
	} else if (strcmp(pair->name, "attribute") == 0) {
		const char *eq = strchr(pair->value, '=');
		if (eq != NULL && eq > pair->value)
			mpd_kvlist_add(&output->attributes,
				       pair->value, eq - pair->value,
				       eq + 1);
	}

	return true;
}

void
mpd_output_free(struct mpd_output *output)
{
	assert(output != NULL);

	free(output->name);
	free(output->plugin);
	mpd_kvlist_deinit(&output->attributes);
	free(output);
}

unsigned
mpd_output_get_id(const struct mpd_output *output)
{
	assert(output != NULL);

	return output->id;
}

const char *
mpd_output_get_name(const struct mpd_output *output)
{
	assert(output != NULL);

	return output->name;
}

const char *
mpd_output_get_plugin(const struct mpd_output *output)
{
	assert(output != NULL);

	return output->plugin;
}

bool
mpd_output_get_enabled(const struct mpd_output *output)
{
	assert(output != NULL);

	return output->enabled;
}

const char *
mpd_output_get_attribute(const struct mpd_output *output, const char *name)
{
	assert(output != NULL);

	return mpd_kvlist_get(&output->attributes, name);
}

const struct mpd_pair *
mpd_output_first_attribute(struct mpd_output *output)
{
	assert(output != NULL);

	return mpd_kvlist_first(&output->attributes);
}

const struct mpd_pair *
mpd_output_next_attribute(struct mpd_output *output)
{
	assert(output != NULL);

	return mpd_kvlist_next(&output->attributes);
}
