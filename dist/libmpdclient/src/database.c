// SPDX-License-Identifier: BSD-2-Clause
// Copyright The Music Player Daemon Project

#include <mpd/database.h>
#include <mpd/send.h>
#include <mpd/recv.h>
#include <mpd/pair.h>
#include <mpd/response.h>
#include "run.h"

#include <stddef.h>
#include <stdlib.h>

bool
mpd_send_list_all(struct mpd_connection *connection, const char *dir)
{
	return mpd_send_command(connection, "listall", dir, NULL);
}

bool
mpd_send_list_all_meta(struct mpd_connection *connection, const char *dir)
{
	return mpd_send_command(connection, "listallinfo", dir, NULL);
}

bool
mpd_send_list_meta(struct mpd_connection *connection, const char *dir)
{
	return mpd_send_command(connection, "lsinfo", dir, NULL);
}

bool
mpd_send_list_files(struct mpd_connection *connection, const char *uri)
{
	return mpd_send_command(connection, "listfiles", uri, NULL);
}

bool
mpd_send_read_comments(struct mpd_connection *connection, const char *path)
{
	return mpd_send_command(connection, "readcomments", path, NULL);
}

bool
mpd_send_update(struct mpd_connection *connection, const char *path)
{
	return mpd_send_command(connection, "update", path, NULL);
}

bool
mpd_send_rescan(struct mpd_connection *connection, const char *path)
{
	return mpd_send_command(connection, "rescan", path, NULL);
}

unsigned
mpd_recv_update_id(struct mpd_connection *connection)
{
	struct mpd_pair *pair;
	unsigned ret = 0;

	pair = mpd_recv_pair_named(connection, "updating_db");
	if (pair != NULL) {
		ret = strtoul(pair->value, NULL, 10);
		mpd_return_pair(connection, pair);
	}

	return ret;
}

unsigned
mpd_run_update(struct mpd_connection *connection, const char *path)
{
	unsigned id;

	if (!mpd_run_check(connection) || !mpd_send_update(connection, path))
		return 0;

	id = mpd_recv_update_id(connection);
	return id != 0 && mpd_response_finish(connection)
		? id : 0;
}

unsigned
mpd_run_rescan(struct mpd_connection *connection, const char *path)
{
	unsigned id;

	if (!mpd_run_check(connection) || !mpd_send_rescan(connection, path))
		return 0;

	id = mpd_recv_update_id(connection);
	return id != 0 && mpd_response_finish(connection)
		? id : 0;
}
