// SPDX-License-Identifier: BSD-2-Clause
// Copyright The Music Player Daemon Project

#include <mpd/mount.h>
#include <mpd/send.h>
#include <mpd/recv.h>
#include <mpd/response.h>
#include "internal.h"
#include "run.h"

#include <stddef.h>

bool
mpd_send_list_mounts(struct mpd_connection *connection)
{
	return mpd_send_command(connection, "listmounts", NULL);
}

struct mpd_mount *
mpd_recv_mount(struct mpd_connection *connection)
{
	struct mpd_mount *mount;
	struct mpd_pair *pair;

	pair = mpd_recv_pair_named(connection, "mount");
	if (pair == NULL)
		return NULL;

	mount = mpd_mount_begin(pair);
	mpd_return_pair(connection, pair);
	if (mount == NULL) {
		mpd_error_code(&connection->error, MPD_ERROR_OOM);
		return NULL;
	}

	while ((pair = mpd_recv_pair(connection)) != NULL &&
	       mpd_mount_feed(mount, pair))
		mpd_return_pair(connection, pair);

	if (mpd_error_is_defined(&connection->error)) {
		assert(pair == NULL);

		mpd_mount_free(mount);
		return NULL;
	}

	mpd_enqueue_pair(connection, pair);
	return mount;
}

bool
mpd_send_mount(struct mpd_connection *connection,
	       const char *uri, const char *storage)
{
	return mpd_send_command(connection, "mount", uri, storage, NULL);
}

bool
mpd_run_mount(struct mpd_connection *connection,
	      const char *uri, const char *storage)
{
	return mpd_run_check(connection) &&
		mpd_send_mount(connection, uri, storage) &&
		mpd_response_finish(connection);
}

bool
mpd_send_unmount(struct mpd_connection *connection, const char *uri)
{
	return mpd_send_command(connection, "unmount", uri, NULL);
}

bool
mpd_run_unmount(struct mpd_connection *connection, const char *uri)
{
	return mpd_run_check(connection) &&
		mpd_send_unmount(connection, uri) &&
		mpd_response_finish(connection);
}
