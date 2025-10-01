// SPDX-License-Identifier: BSD-3-Clause
// Copyright The Music Player Daemon Project

#include <mpd/send.h>
#include <mpd/recv.h>
#include <mpd/pair.h>
#include <mpd/response.h>
#include <mpd/connection.h>
#include <mpd/albumart.h>
#include "run.h"
#include "internal.h"
#include "sync.h"
#include "isend.h"

#include <string.h>
#include <stdlib.h>
#include <inttypes.h>

bool
mpd_send_albumart(struct mpd_connection *connection, const char *uri, unsigned offset)
{
	return mpd_send_s_u_command(connection, "albumart", uri, offset);
}

int
mpd_recv_albumart(struct mpd_connection *connection, void *buffer, size_t buffer_size)
{
	struct mpd_pair *pair = mpd_recv_pair_named(connection, "binary");
	if (pair == NULL) {
		return -1;
	}

	size_t chunk_size = strtoumax(pair->value, NULL, 10);
	mpd_return_pair(connection, pair);

	size_t retrieve_bytes = chunk_size > buffer_size ? buffer_size : chunk_size;
	if (mpd_recv_binary(connection, buffer, retrieve_bytes) == false) {
		return -1;
	}

	return (int)retrieve_bytes;
}

int
mpd_run_albumart(struct mpd_connection *connection,
				 const char *uri, unsigned offset,
				 void *buffer, size_t buffer_size)
{
	if (!mpd_run_check(connection) ||
		!mpd_send_albumart(connection, uri, offset)) {
			return -1;
	}

	int read_size = mpd_recv_albumart(connection, buffer, buffer_size);
	if (!mpd_response_finish(connection)) {
		return -1;
	}

	return read_size;
}
