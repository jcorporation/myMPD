// SPDX-License-Identifier: BSD-2-Clause
// Copyright The Music Player Daemon Project

#include <mpd/playlist.h>
#include <mpd/position.h>
#include <mpd/send.h>
#include <mpd/response.h>
#include "isend.h"
#include "run.h"

#include <limits.h>
#include <stdio.h>
#include <string.h>

/* (bits+1)/3 (plus the sign character) */
enum {
	UNSIGNEDLEN = (sizeof(unsigned) * CHAR_BIT + 1) / 3 + 1,
};

bool
mpd_send_list_playlists(struct mpd_connection *connection)
{
	return mpd_send_command(connection, "listplaylists", NULL);
}

bool
mpd_send_list_playlist(struct mpd_connection *connection, const char *name)
{
	return mpd_send_command(connection, "listplaylist", name, NULL);
}

bool
mpd_send_list_playlist_range(struct mpd_connection *connection, const char *name,
			     unsigned start, unsigned end)
{
	return mpd_send_s_range_command(connection, "listplaylist", name, start, end);
}

bool
mpd_send_list_playlist_meta(struct mpd_connection *connection, const char *name)
{
	return mpd_send_command(connection, "listplaylistinfo", name, NULL);
}

bool
mpd_send_list_playlist_range_meta(struct mpd_connection *connection, const char *name,
				  unsigned start, unsigned end)
{
	return mpd_send_s_range_command(connection, "listplaylistinfo", name, start, end);
}

bool
mpd_send_playlist_clear(struct mpd_connection *connection, const char *name)
{
	return mpd_send_command(connection, "playlistclear", name, NULL);
}

bool
mpd_run_playlist_clear(struct mpd_connection *connection, const char *name)
{
	return mpd_run_check(connection) &&
		mpd_send_playlist_clear(connection, name) &&
		mpd_response_finish(connection);
}

bool
mpd_send_playlist_add(struct mpd_connection *connection, const char *name,
		      const char *path)
{
	return mpd_send_command(connection, "playlistadd", name, path, NULL);
}

bool
mpd_run_playlist_add(struct mpd_connection *connection,
		     const char *name, const char *path)
{
	return mpd_run_check(connection) &&
		mpd_send_playlist_add(connection, name, path) &&
		mpd_response_finish(connection);
}

bool
mpd_send_playlist_add_to(struct mpd_connection *connection, const char *name,
		      const char *path, unsigned to)
{
	return mpd_send_s_s_u_command(connection, "playlistadd", name, path, to);
}

bool
mpd_run_playlist_add_to(struct mpd_connection *connection,
		     const char *name, const char *path, unsigned to)
{
	return mpd_run_check(connection) &&
		mpd_send_playlist_add_to(connection, name, path, to) &&
		mpd_response_finish(connection);
}

bool
mpd_send_playlist_move(struct mpd_connection *connection, const char *name,
		       unsigned from, unsigned to)
{
	char from_string[UNSIGNEDLEN], to_string[UNSIGNEDLEN];

	snprintf(from_string, sizeof(from_string), "%u", from);
	snprintf(to_string, sizeof(to_string), "%u", to);

	return mpd_send_command(connection, "playlistmove", name,
				from_string, to_string, NULL);
}

bool
mpd_run_playlist_move(struct mpd_connection *connection, const char *name,
		       unsigned from, unsigned to)
{
	return mpd_run_check(connection) &&
		mpd_send_playlist_move(connection, name, from, to) &&
		mpd_response_finish(connection);
}

bool
mpd_send_playlist_move_range(struct mpd_connection *connection, const char *name,
		       unsigned start, unsigned end, unsigned to)
{
	return mpd_send_s_range_to_u_command(connection, "playlistmove", name,
				start, end, to);
}

bool
mpd_run_playlist_move_range(struct mpd_connection *connection, const char *name,
		       unsigned start, unsigned end, unsigned to)
{
	return mpd_run_check(connection) &&
		mpd_send_playlist_move_range(connection, name, start, end, to) &&
		mpd_response_finish(connection);
}

bool
mpd_send_playlist_delete(struct mpd_connection *connection, const char *name,
			 unsigned pos)
{
	char pos_string[UNSIGNEDLEN];

	snprintf(pos_string, sizeof(pos_string), "%u", pos);

	return mpd_send_command(connection, "playlistdelete", name, pos_string, NULL);
}

bool
mpd_run_playlist_delete(struct mpd_connection *connection,
			const char *name, unsigned pos)
{
	return mpd_run_check(connection) &&
		mpd_send_playlist_delete(connection, name, pos) &&
		mpd_response_finish(connection);
}

bool
mpd_send_playlist_delete_range(struct mpd_connection *connection, const char *name,
			 unsigned start, unsigned end)
{
	return mpd_send_s_range_command(connection, "playlistdelete", name,
					start, end);
}

bool
mpd_run_playlist_delete_range(struct mpd_connection *connection,
			const char *name, unsigned start, unsigned end)
{
	return mpd_run_check(connection) &&
		mpd_send_playlist_delete_range(connection, name, start, end) &&
		mpd_response_finish(connection);
}

bool
mpd_send_save(struct mpd_connection *connection, const char *name)
{
	return mpd_send_command(connection, "save", name, NULL);
}

bool
mpd_run_save(struct mpd_connection *connection, const char *name)
{
	return mpd_run_check(connection) &&
		mpd_send_save(connection, name) &&
		mpd_response_finish(connection);
}

enum mpd_queue_save_mode mpd_parse_queue_save_mode(const char *p)
{
	if (strcmp(p, "create") == 0)
		return MPD_QUEUE_SAVE_MODE_CREATE;
	else if (strcmp(p, "replace") == 0)
		return MPD_QUEUE_SAVE_MODE_REPLACE;
	else if (strcmp(p, "append") == 0)
		return MPD_QUEUE_SAVE_MODE_APPEND;
	else
		return MPD_QUEUE_SAVE_MODE_UNKNOWN;
}

const char *
mpd_lookup_queue_save_mode(enum mpd_queue_save_mode mode)
{
	switch (mode) {
	case MPD_QUEUE_SAVE_MODE_CREATE:
		return "create";
	case MPD_QUEUE_SAVE_MODE_REPLACE:
		return "replace";
	case MPD_QUEUE_SAVE_MODE_APPEND:
		return "append";
	case MPD_QUEUE_SAVE_MODE_UNKNOWN:
		return NULL;
	}
	return NULL;
}

bool
mpd_send_save_queue(struct mpd_connection *connection, const char *name,
		enum mpd_queue_save_mode mode)
{
	const char *mode_str = mpd_lookup_queue_save_mode(mode);
	if (mode_str == NULL)
		return false;
	return mpd_send_command(connection, "save", name, mode_str, NULL);
}

bool
mpd_run_save_queue(struct mpd_connection *connection, const char *name,
		enum mpd_queue_save_mode mode)
{
	return mpd_run_check(connection) &&
		mpd_send_save_queue(connection, name, mode) &&
		mpd_response_finish(connection);
}

bool
mpd_send_load(struct mpd_connection *connection, const char *name)
{
	return mpd_send_command(connection, "load", name, NULL);
}

bool
mpd_run_load(struct mpd_connection *connection, const char *name)
{
	return mpd_run_check(connection) &&
		mpd_send_load(connection, name) &&
		mpd_response_finish(connection);
}

bool
mpd_send_load_range(struct mpd_connection *connection, const char *name,
		    unsigned start, unsigned end)
{
	return mpd_send_s_range_command(connection, "load", name,
					start, end);
}

bool
mpd_run_load_range(struct mpd_connection *connection, const char *name,
		   unsigned start, unsigned end)
{
	return mpd_run_check(connection) &&
		mpd_send_load_range(connection, name, start, end) &&
		mpd_response_finish(connection);
}

bool
mpd_send_load_range_to(struct mpd_connection *connection, const char *name,
		    unsigned start, unsigned end, unsigned to, enum mpd_position_whence whence)
{
	const char *whence_s = mpd_position_whence_char(whence);
	char to_str[64] = "";
	snprintf(to_str, 64, "%s%u", whence_s, to);

	return mpd_send_s_range_to_command(connection, "load", name,
					start, end, to_str);
}

bool
mpd_run_load_range_to(struct mpd_connection *connection, const char *name,
		   unsigned start, unsigned end, unsigned to, enum mpd_position_whence whence)
{
	return mpd_run_check(connection) &&
		mpd_send_load_range_to(connection, name, start, end, to, whence) &&
		mpd_response_finish(connection);
}

bool
mpd_send_rename(struct mpd_connection *connection,
		const char *from, const char *to)
{
	return mpd_send_command(connection, "rename", from, to, NULL);
}

bool
mpd_run_rename(struct mpd_connection *connection,
	       const char *from, const char *to)
{
	return mpd_run_check(connection) &&
		mpd_send_rename(connection, from, to) &&
		mpd_response_finish(connection);
}

bool
mpd_send_rm(struct mpd_connection *connection, const char *name)
{
	return mpd_send_command(connection, "rm", name, NULL);
}

bool
mpd_run_rm(struct mpd_connection *connection, const char *name)
{
	return mpd_run_check(connection) &&
		mpd_send_rm(connection, name) &&
		mpd_response_finish(connection);
}

bool
mpd_send_playlistlength(struct mpd_connection *connection, const char *name)
{
	return mpd_send_command(connection, "playlistlength", name, NULL);
}
