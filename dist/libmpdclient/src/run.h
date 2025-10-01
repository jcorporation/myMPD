// SPDX-License-Identifier: BSD-2-Clause
// Copyright The Music Player Daemon Project

#ifndef MPD_RUN_H
#define MPD_RUN_H

#include <stdbool.h>

struct mpd_connection;

/**
 * Check if it's possible to run a single command via mpd_run_X().
 * This is not possible if the connection is currently sending a
 * command list.
 *
 * @return true if that's possible, and false if not (error set
 * accordingly)
 */
bool
mpd_run_check(struct mpd_connection *connection);

#endif
