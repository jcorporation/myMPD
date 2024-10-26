// SPDX-License-Identifier: BSD-3-Clause
// Copyright The Music Player Daemon Project

#ifndef MPD_ISEARCH_H
#define MPD_ISEARCH_H

#include <stdbool.h>
#include <stdlib.h>

struct mpd_connection;

char *
mpd_sanitize_arg(const char *src);

bool
mpd_request_begin(struct mpd_connection *connection);

bool
mpd_request_command(struct mpd_connection *connection, const char *cmd);

char *
mpd_request_prepare_append(struct mpd_connection *connection,
			   size_t add_length);

bool
mpd_request_add_sort(struct mpd_connection *connection,
		     const char *name, bool descending);

bool
mpd_request_add_window(struct mpd_connection *connection,
		       unsigned start, unsigned end);

bool
mpd_request_commit(struct mpd_connection *connection);

void
mpd_request_cancel(struct mpd_connection *connection);

#endif
