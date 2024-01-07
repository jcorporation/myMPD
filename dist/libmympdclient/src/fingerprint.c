// SPDX-License-Identifier: BSD-3-Clause
// Copyright The Music Player Daemon Project

#include <mpd/fingerprint.h>
#include <mpd/send.h>
#include <mpd/recv.h>
#include <mpd/pair.h>
#include <mpd/response.h>
#include "run.h"

#include <string.h>
#include <stdio.h>

bool
mpd_send_getfingerprint(struct mpd_connection *connection, const char *uri)
{
	return mpd_send_command(connection, "getfingerprint", uri, NULL);
}

enum mpd_fingerprint_type
mpd_parse_fingerprint_type(const char *name)
{
	if (strcmp(name, "chromaprint") == 0)
		return MPD_FINGERPRINT_TYPE_CHROMAPRINT;
	else
		return MPD_FINGERPRINT_TYPE_UNKNOWN;
}

const char *
mpd_run_getfingerprint_chromaprint(struct mpd_connection *connection,
				   const char *uri,
				   char *buffer, size_t buffer_size)
{
	if (!mpd_run_check(connection) ||
	    !mpd_send_getfingerprint(connection, uri))
		return NULL;

	const char *result = NULL;

	struct mpd_pair *pair = mpd_recv_pair_named(connection, "chromaprint");
	if (pair != NULL) {
		snprintf(buffer, buffer_size, "%s", pair->value);
		result = buffer;
		mpd_return_pair(connection, pair);
	}

	if (!mpd_response_finish(connection))
		result = NULL;

	return result;
}
