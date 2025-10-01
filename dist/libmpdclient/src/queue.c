// SPDX-License-Identifier: BSD-2-Clause
// Copyright The Music Player Daemon Project

#include <mpd/queue.h>
#include <mpd/send.h>
#include <mpd/recv.h>
#include <mpd/pair.h>
#include <mpd/response.h>
#include <mpd/song.h>
#include "internal.h"
#include "isend.h"
#include "run.h"

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>

bool
mpd_send_list_queue_meta(struct mpd_connection *connection)
{
	return mpd_send_command(connection, "playlistinfo", NULL);
}

bool
mpd_send_list_queue_range_meta(struct mpd_connection *connection,
			       unsigned start, unsigned end)
{
	return mpd_send_range_command(connection, "playlistinfo", start, end);
}

bool
mpd_send_get_queue_song_pos(struct mpd_connection *connection, unsigned pos)
{
	return mpd_send_u_command(connection, "playlistinfo", pos);
}

struct mpd_song *
mpd_run_get_queue_song_pos(struct mpd_connection *connection, unsigned pos)
{
	struct mpd_song *song;

	if (!mpd_run_check(connection) ||
	    !mpd_send_get_queue_song_pos(connection, pos))
		return NULL;

	song = mpd_recv_song(connection);
	if (!mpd_response_finish(connection) && song != NULL) {
		mpd_song_free(song);
		return NULL;
	}

	return song;

}

bool
mpd_send_get_queue_song_id(struct mpd_connection *connection, unsigned id)
{
	return mpd_send_u_command(connection, "playlistid", id);
}

struct mpd_song *
mpd_run_get_queue_song_id(struct mpd_connection *connection, unsigned id)
{
	struct mpd_song *song;

	if (!mpd_run_check(connection) ||
	    !mpd_send_get_queue_song_id(connection, id))
		return NULL;

	song = mpd_recv_song(connection);
	if (!mpd_response_finish(connection) && song != NULL) {
		mpd_song_free(song);
		return NULL;
	}

	return song;

}

bool
mpd_send_queue_changes_meta(struct mpd_connection *connection,
			    unsigned version)
{
	return mpd_send_u_command(connection, "plchanges", version);
}

bool
mpd_send_queue_changes_meta_range(struct mpd_connection *connection,
				  unsigned version,
				  unsigned start, unsigned end)
{
	return mpd_send_u_range_command(connection, "plchanges",
					version, start, end);
}

bool
mpd_send_queue_changes_brief(struct mpd_connection *connection,
			     unsigned version)
{
	return mpd_send_u_command(connection, "plchangesposid", version);
}

bool
mpd_send_queue_changes_brief_range(struct mpd_connection *connection,
				   unsigned version,
				   unsigned start, unsigned end)
{
	return mpd_send_u_range_command(connection, "plchangesposid",
					version, start, end);
}

bool
mpd_recv_queue_change_brief(struct mpd_connection *connection,
			    unsigned *position_r, unsigned *id_r)
{
	struct mpd_pair *pair;

	pair = mpd_recv_pair_named(connection, "cpos");
	if (pair == NULL)
		return false;

	*position_r = strtoul(pair->value, NULL, 10);
	mpd_return_pair(connection, pair);

	pair = mpd_recv_pair_named(connection, "Id");
	if (pair == NULL) {
		mpd_return_pair(connection, pair);

		if (!mpd_error_is_defined(&connection->error)) {
			mpd_error_code(&connection->error,
				       MPD_ERROR_MALFORMED);
			mpd_error_message(&connection->error,
					  "No id received");
		}

		return false;
	}

	*id_r = strtoul(pair->value, NULL, 10);
	mpd_return_pair(connection, pair);

	return !mpd_error_is_defined(&connection->error);
}

bool
mpd_send_add(struct mpd_connection *connection, const char *uri)
{
	return mpd_send_command(connection, "add", uri, NULL);
}

bool
mpd_run_add(struct mpd_connection *connection, const char *uri)
{
	return mpd_run_check(connection) &&
		mpd_send_add(connection, uri) &&
		mpd_response_finish(connection);
}

bool
mpd_send_add_whence(struct mpd_connection *connection, const char *uri,
		   unsigned to, enum mpd_position_whence whence)
{
	const char *whence_s = mpd_position_whence_char(whence);

	char to_str[64] = "";
	snprintf(to_str, 64, "%s%u", whence_s, to);

	return mpd_send_s_s_command(connection, "add", uri, to_str);
}

bool
mpd_run_add_whence(struct mpd_connection *connection, const char *uri,
		  unsigned to, enum mpd_position_whence whence)
{
	return mpd_run_check(connection) &&
		mpd_send_add_whence(connection, uri, to, whence) &&
		mpd_response_finish(connection);
}

bool
mpd_send_add_id(struct mpd_connection *connection, const char *uri)
{
	return mpd_send_command(connection, "addid", uri, NULL);
}

bool
mpd_send_add_id_to(struct mpd_connection *connection, const char *uri,
		   unsigned to)
{
	return mpd_send_s_u_command(connection, "addid", uri, to);
}

bool
mpd_send_add_id_whence(struct mpd_connection *connection, const char *uri,
		   unsigned to, enum mpd_position_whence whence)
{
	const char *whence_s = mpd_position_whence_char(whence);

	char to_str[64] = "";
	snprintf(to_str, 64, "%s%u", whence_s, to);

	return mpd_send_s_s_command(connection, "addid", uri, to_str);
}

int
mpd_recv_song_id(struct mpd_connection *connection)
{
	struct mpd_pair *pair;
	int id = -1;

	pair = mpd_recv_pair_named(connection, "Id");
	if (pair != NULL) {
		id = atoi(pair->value);
		mpd_return_pair(connection, pair);
	}

	return id;
}

int
mpd_run_add_id(struct mpd_connection *connection, const char *uri)
{
	int id;

	if (!mpd_run_check(connection) ||
	    !mpd_send_add_id(connection, uri))
		return -1;

	id = mpd_recv_song_id(connection);

	if (!mpd_response_finish(connection))
		id = -1;

	return id;
}

int
mpd_run_add_id_to(struct mpd_connection *connection, const char *uri,
		  unsigned to)
{
	int id;

	if (!mpd_run_check(connection) ||
	    !mpd_send_add_id_to(connection, uri, to))
		return -1;

	id = mpd_recv_song_id(connection);

	if (!mpd_response_finish(connection))
		id = -1;

	return id;
}

int
mpd_run_add_id_whence(struct mpd_connection *connection, const char *uri,
		  unsigned to, enum mpd_position_whence whence)
{
	int id;

	if (!mpd_run_check(connection) ||
	    !mpd_send_add_id_whence(connection, uri, to, whence))
		return -1;

	id = mpd_recv_song_id(connection);

	if (!mpd_response_finish(connection))
		id = -1;

	return id;
}

bool
mpd_send_delete(struct mpd_connection *connection, unsigned pos)
{
	return mpd_send_u_command(connection, "delete", pos);
}

bool
mpd_run_delete(struct mpd_connection *connection, unsigned pos)
{
	return mpd_run_check(connection) &&
		mpd_send_delete(connection, pos) &&
		mpd_response_finish(connection);
}

bool
mpd_send_delete_range(struct mpd_connection *connection,
		      unsigned start, unsigned end)
{
	return mpd_send_range_command(connection, "delete", start, end);
}

bool
mpd_run_delete_range(struct mpd_connection *connection,
		     unsigned start, unsigned end)
{
	return mpd_run_check(connection) &&
		mpd_send_delete_range(connection, start, end) &&
		mpd_response_finish(connection);
}

bool
mpd_send_delete_id(struct mpd_connection *connection, unsigned id)
{
	return mpd_send_u_command(connection, "deleteid", id);
}

bool
mpd_run_delete_id(struct mpd_connection *connection, unsigned id)
{
	return mpd_run_check(connection) &&
		mpd_send_delete_id(connection, id) &&
		mpd_response_finish(connection);
}

bool
mpd_send_shuffle(struct mpd_connection *connection)
{
	return mpd_send_command(connection, "shuffle", NULL);
}

bool
mpd_run_shuffle(struct mpd_connection *connection)
{
	return mpd_run_check(connection) &&
		mpd_send_shuffle(connection) &&
		mpd_response_finish(connection);
}

bool
mpd_send_shuffle_range(struct mpd_connection *connection, unsigned start, unsigned end)
{
	return mpd_send_range_command(connection, "shuffle", start, end);
}

bool
mpd_run_shuffle_range(struct mpd_connection *connection,
		      unsigned start, unsigned end)
{
	return mpd_run_check(connection) &&
		mpd_send_shuffle_range(connection, start, end) &&
		mpd_response_finish(connection);
}

bool
mpd_send_clear(struct mpd_connection *connection)
{
	return mpd_send_command(connection, "clear", NULL);
}

bool
mpd_run_clear(struct mpd_connection *connection)
{
	return mpd_run_check(connection) &&
		mpd_send_clear(connection) &&
		mpd_response_finish(connection);
}

bool
mpd_send_move(struct mpd_connection *connection, unsigned from, unsigned to)
{
	return mpd_send_u2_command(connection, "move", from, to);
}

bool
mpd_run_move(struct mpd_connection *connection, unsigned from, unsigned to)
{
	return mpd_run_check(connection) &&
		mpd_send_move(connection, from, to) &&
		mpd_response_finish(connection);
}

bool
mpd_send_move_whence(struct mpd_connection *connection, unsigned from,
			 unsigned to, enum mpd_position_whence whence)
{
	const char *whence_s = mpd_position_whence_char(whence);

	char to_str[64] = "";
	snprintf(to_str, 64, "%s%u", whence_s, to);

	return mpd_send_u_s_command(connection, "move", from, to_str);
}

bool
mpd_run_move_whence(struct mpd_connection *connection, unsigned from,
			 unsigned to, enum mpd_position_whence whence)
{
	return mpd_run_check(connection) &&
		mpd_send_move_whence(connection, from, to, whence) &&
		mpd_response_finish(connection);
}

bool
mpd_send_move_id(struct mpd_connection *connection, unsigned from, unsigned to)
{
	return mpd_send_u2_command(connection, "moveid", from, to);
}

bool
mpd_run_move_id(struct mpd_connection *connection, unsigned from, unsigned to)
{
	return mpd_run_check(connection) &&
		mpd_send_move_id(connection, from, to) &&
		mpd_response_finish(connection);
}

bool
mpd_send_move_id_whence(struct mpd_connection *connection, unsigned from,
			 unsigned to, enum mpd_position_whence whence)
{
	const char *whence_s = mpd_position_whence_char(whence);

	char to_str[64] = "";
	snprintf(to_str, 64, "%s%u", whence_s, to);

	return mpd_send_u_s_command(connection, "moveid", from, to_str);
}

bool
mpd_run_move_id_whence(struct mpd_connection *connection, unsigned from,
			 unsigned to, enum mpd_position_whence whence)
{
	return mpd_run_check(connection) &&
		mpd_send_move_id_whence(connection, from, to, whence) &&
		mpd_response_finish(connection);
}

bool
mpd_send_move_range(struct mpd_connection *connection,
		    unsigned start, unsigned end, unsigned to)
{
	return mpd_send_range_u_command(connection, "move", start, end, to);
}

bool
mpd_run_move_range(struct mpd_connection *connection,
		   unsigned start, unsigned end, unsigned to)
{
	return mpd_run_check(connection) &&
		mpd_send_move_range(connection, start, end, to) &&
		mpd_response_finish(connection);
}

bool
mpd_send_move_range_whence(struct mpd_connection *connection, unsigned start,
			 unsigned end, unsigned to, enum mpd_position_whence whence)
{
	const char *whence_s = mpd_position_whence_char(whence);

	char to_str[64] = "";
	snprintf(to_str, 64, "%s%u", whence_s, to);

	return mpd_send_range_to_command(connection, "move", start, end, to_str);
}

bool
mpd_run_move_range_whence(struct mpd_connection *connection, unsigned start,
			 unsigned end, unsigned to, enum mpd_position_whence whence)
{
	return mpd_run_check(connection) &&
		mpd_send_move_range_whence(connection, start, end, to, whence) &&
		mpd_response_finish(connection);
}

bool
mpd_send_swap(struct mpd_connection *connection, unsigned pos1, unsigned pos2)
{
	return mpd_send_u2_command(connection, "swap", pos1, pos2);
}

bool
mpd_run_swap(struct mpd_connection *connection, unsigned pos1, unsigned pos2)
{
	return mpd_run_check(connection) &&
		mpd_send_swap(connection, pos1, pos2) &&
		mpd_response_finish(connection);
}

bool
mpd_send_swap_id(struct mpd_connection *connection, unsigned id1, unsigned id2)
{
	return mpd_send_u2_command(connection, "swapid", id1, id2);
}

bool
mpd_run_swap_id(struct mpd_connection *connection, unsigned id1, unsigned id2)
{
	return mpd_run_check(connection) &&
		mpd_send_swap_id(connection, id1, id2) &&
		mpd_response_finish(connection);
}

bool
mpd_send_add_tag_id(struct mpd_connection *connection, unsigned id,
		    enum mpd_tag_type tag, const char *value)
{
	return mpd_send_u_s_s_command(connection, "addtagid",
				      id, mpd_tag_name(tag), value);
}

bool
mpd_run_add_tag_id(struct mpd_connection *connection, unsigned id,
		   enum mpd_tag_type tag, const char *value)
{
	return mpd_run_check(connection) &&
		mpd_send_add_tag_id(connection, id, tag, value) &&
		mpd_response_finish(connection);
}

bool
mpd_send_clear_tag_id(struct mpd_connection *connection, unsigned id,
		      enum mpd_tag_type tag)
{
	return mpd_send_u_s_command(connection, "cleartagid",
				    id, mpd_tag_name(tag));
}

bool
mpd_run_clear_tag_id(struct mpd_connection *connection, unsigned id,
		     enum mpd_tag_type tag)
{
	return mpd_run_check(connection) &&
		mpd_send_clear_tag_id(connection, id, tag) &&
		mpd_response_finish(connection);
}

bool
mpd_send_clear_all_tags_id(struct mpd_connection *connection, unsigned id)
{
	return mpd_send_u_command(connection, "cleartagid", id);
}

bool
mpd_run_clear_all_tags_id(struct mpd_connection *connection, unsigned id)
{
	return mpd_run_check(connection) &&
		mpd_send_clear_all_tags_id(connection, id) &&
		mpd_response_finish(connection);
}

bool
mpd_send_prio(struct mpd_connection *connection, unsigned priority,
	      unsigned position)
{
	return mpd_send_u2_command(connection, "prio", priority, position);
}

bool
mpd_run_prio(struct mpd_connection *connection, unsigned priority,
	     unsigned position)
{
	return mpd_run_check(connection) &&
		mpd_send_prio(connection, priority, position) &&
		mpd_response_finish(connection);
}

bool
mpd_send_prio_range(struct mpd_connection *connection, unsigned priority,
		    unsigned start, unsigned end)
{
	return mpd_send_u_range_command(connection, "prio", priority,
					start, end);
}

bool
mpd_run_prio_range(struct mpd_connection *connection, unsigned priority,
		   unsigned start, unsigned end)
{
	return mpd_run_check(connection) &&
		mpd_send_prio_range(connection, priority, start, end) &&
		mpd_response_finish(connection);
}

bool
mpd_send_prio_id(struct mpd_connection *connection, unsigned priority,
		 unsigned id)
{
	return mpd_send_u2_command(connection, "prioid", priority, id);
}

bool
mpd_run_prio_id(struct mpd_connection *connection, unsigned priority,
		unsigned id)
{
	return mpd_run_check(connection) &&
		mpd_send_prio_id(connection, priority, id) &&
		mpd_response_finish(connection);
}

bool
mpd_send_range_id(struct mpd_connection *connection, unsigned id,
		  float start, float end)
{
	return mpd_send_u_frange_command(connection, "rangeid", id, start, end);
}

bool
mpd_run_range_id(struct mpd_connection *connection, unsigned id,
		 float start, float end)
{
	return mpd_run_check(connection) &&
		mpd_send_range_id(connection, id, start, end) &&
		mpd_response_finish(connection);
}
