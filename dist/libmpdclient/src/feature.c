// SPDX-License-Identifier: BSD-3-Clause
// Copyright The Music Player Daemon Project

#include <mpd/feature.h>

#include <assert.h>
#include <string.h>
#include <stdbool.h>

static const char *const mpd_feature_names[MPD_FEATURE_COUNT] =
{
	[MPD_FEATURE_HIDE_PLAYLISTS_IN_ROOT] = "hide_playlists_in_root",
};

const char *
mpd_feature_name(enum mpd_protocol_feature feature)
{
	if ((unsigned)feature >= MPD_FEATURE_COUNT)
		return NULL;

	return mpd_feature_names[feature];
}

enum mpd_protocol_feature
mpd_feature_name_parse(const char *name)
{
	assert(name != NULL);

	for (unsigned i = 0; i < MPD_FEATURE_COUNT; ++i)
		if (strcmp(name, mpd_feature_names[i]) == 0)
			return (enum mpd_protocol_feature)i;

	return MPD_FEATURE_UNKNOWN;
}
