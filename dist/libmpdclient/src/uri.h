// SPDX-License-Identifier: BSD-2-Clause
// Copyright The Music Player Daemon Project

#ifndef MPD_URI_H
#define MPD_URI_H

#include <assert.h>
#include <string.h>

static inline bool
mpd_verify_uri(const char *uri)
{
	assert(uri != NULL);

	return *uri != 0;
}

static inline bool
mpd_verify_local_uri(const char *uri)
{
	assert(uri != NULL);

	return mpd_verify_uri(uri) && *uri != '/' &&
		uri[strlen(uri) - 1] != '/';
}

#endif
