// SPDX-License-Identifier: BSD-3-Clause
// Copyright The Music Player Daemon Project

#include "check_tag.h"
#include "ierror.h"

#include <assert.h>

const char *
mpd_check_tag_name(enum mpd_tag_type type, struct mpd_error_info *error)
{
	assert(error != NULL);

	const char *name = mpd_tag_name(type);
	if (name == NULL) {
		mpd_error_code(error, MPD_ERROR_ARGUMENT);
		mpd_error_message(error, "invalid tag type");
	}

	return name;
}
