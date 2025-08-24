// SPDX-License-Identifier: BSD-2-Clause
// Copyright The Music Player Daemon Project

#include <mpd/partition.h>
#include <mpd/send.h>
#include <mpd/response.h>
#include "internal.h"
#include "run.h"

#include <assert.h>
#include <stddef.h>

bool
mpd_send_newpartition(struct mpd_connection *connection, const char *partition)
{
	return mpd_send_command(connection, "newpartition", partition, NULL);
}

bool
mpd_run_newpartition(struct mpd_connection *connection, const char *partition)
{
	return mpd_run_check(connection) &&
		mpd_send_newpartition(connection, partition) &&
		mpd_response_finish(connection);
}

bool
mpd_send_delete_partition(struct mpd_connection *connection,
			  const char *partition)
{
	return mpd_send_command(connection, "delpartition", partition, NULL);
}

bool
mpd_run_delete_partition(struct mpd_connection *connection,
			 const char *partition)
{
	return mpd_run_check(connection) &&
		mpd_send_delete_partition(connection, partition) &&
		mpd_response_finish(connection);
}

bool
mpd_send_switch_partition(struct mpd_connection *connection,
			  const char *partition)
{
	return mpd_send_command(connection, "partition", partition, NULL);
}

bool
mpd_run_switch_partition(struct mpd_connection *connection,
			 const char *partition)
{
	return mpd_run_check(connection) &&
		mpd_send_switch_partition(connection, partition) &&
		mpd_response_finish(connection);
}

bool
mpd_send_listpartitions(struct mpd_connection *connection)
{
	return mpd_send_command(connection, "listpartitions", NULL);
}

struct mpd_partition *
mpd_recv_partition(struct mpd_connection *connection)
{
	struct mpd_pair *pair = mpd_recv_partition_pair(connection);
	if (pair == NULL)
		return NULL;

	struct mpd_partition *partition = mpd_partition_new(pair);
	mpd_return_pair(connection, pair);
	return partition;
}
