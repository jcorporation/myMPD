// SPDX-License-Identifier: BSD-2-Clause
// Copyright The Music Player Daemon Project

/*! \file
 * \brief MPD client library
 *
 * Manipulate the queue (current playlist).
 *
 * Do not include this header directly.  Use mpd/client.h instead.
 */

#ifndef MPD_QUEUE_H
#define MPD_QUEUE_H

#include "compiler.h"
#include "position.h"
#include "tag.h"

#include <stdbool.h>

struct mpd_connection;

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Consult mpd/playlist.h for the rationale on the preference of manipulating
 * song ids over positions in the queue.
 */

/**
 * Sends the "playlistinfo" command: list all songs in the queue
 * including meta information.
 * Use mpd_recv_entity() to receive the songs (#MPD_ENTITY_TYPE_SONG).
 *
 * @param connection the connection to MPD
 * @return true on success, false on error
 */
bool
mpd_send_list_queue_meta(struct mpd_connection *connection);

/**
 * Like mpd_send_list_queue_meta(), but specifies a (position) range.
 * Use mpd_recv_entity() to receive the songs (#MPD_ENTITY_TYPE_SONG).
 *
 * @param connection the connection to MPD
 * @param start the start position of the range (including)
 * @param end the end position of the range (excluding); the special
 * value "UINT_MAX" makes the end of the range open
 * @return true on success, false on error
 *
 * @since libmpdclient 2.8 added support for "UINT_MAX"
 */
bool
mpd_send_list_queue_range_meta(struct mpd_connection *connection,
			       unsigned start, unsigned end);

/**
 * Requests information (including tags) about one song in the
 * playlist (command "playlistid").
 * Use mpd_recv_song() to obtain the song information.
 *
 * @param connection the connection to MPD
 * @param pos the position of the requested song
 */
bool
mpd_send_get_queue_song_pos(struct mpd_connection *connection, unsigned pos);

/**
 * Shortcut for mpd_send_get_queue_song_pos() and mpd_recv_song().
 *
 * @param connection the connection to MPD
 * @param pos the position of the requested song
 * @return the song at the specified position, or NULL on error
 */
mpd_malloc
struct mpd_song *
mpd_run_get_queue_song_pos(struct mpd_connection *connection, unsigned pos);

/**
 * Requests information (including tags) about one song in the
 * playlist (command "playlistid").
 * Use mpd_recv_song() to obtain the song information.
 *
 * @param connection the connection to MPD
 * @param id the id of the requested song
 */
bool
mpd_send_get_queue_song_id(struct mpd_connection *connection, unsigned id);

/**
 * Shortcut for mpd_send_get_queue_song_id() and mpd_recv_song().
 *
 * @param connection the connection to MPD
 * @param id the id of the requested song
 * @return the song at the specified id, or NULL on error
 */
mpd_malloc
struct mpd_song *
mpd_run_get_queue_song_id(struct mpd_connection *connection, unsigned id);

/**
 * Request the queue changes from MPD since the specified version,
 * including tags.  The MPD command is called "plchanges".
 *
 * The current version can be fetched with mpd_status_get_queue_version().
 * Use mpd_recv_song() to receive the songs of the new version.
 *
 * @param connection the connection to MPD
 * @param version The playlist version you want the diff with.
 * @return true on success, false on error
 */
bool
mpd_send_queue_changes_meta(struct mpd_connection *connection,
			    unsigned version);

/**
 * Same as mpd_send_queue_changes_meta(), but limit the result to a
 * range.
 *
 * The current version can be fetched with mpd_status_get_queue_version().
 * Use mpd_recv_song() to receive the songs of the new queue.
 *
 * @param connection the connection to MPD
 * @param version The playlist version you want the diff with.
 * @param start the start position of the range (including)
 * @param end the end position of the range (excluding); the special
 * value "UINT_MAX" makes the end of the range open
 * @return true on success, false on error
 *
 * @since libmpdclient 2.12
 */
bool
mpd_send_queue_changes_meta_range(struct mpd_connection *connection,
				  unsigned version,
				  unsigned start, unsigned end);

/**
 * A more bandwidth efficient version of the
 * mpd_send_queue_changes_meta().  It only returns the position and id
 * of changed songs.  The MPD command is called "plchangesposid".
 *
 * Use mpd_recv_queue_change_brief() for the response.
 *
 * @param connection A valid and connected mpd_connection.
 * @param version The playlist version you want the diff with.
 * @return true on success, false on error
 */
bool
mpd_send_queue_changes_brief(struct mpd_connection *connection,
			     unsigned version);

/**
 * Same as mpd_send_queue_changes_brief(), but limit the result to a
 * range.
 *
 * Use mpd_recv_queue_change_brief() for the response.
 *
 * @param connection the connection to MPD
 * @param version The playlist version you want the diff with.
 * @param start the start position of the range (including)
 * @param end the end position of the range (excluding); the special
 * value "UINT_MAX" makes the end of the range open
 * @return true on success, false on error
 *
 * @since libmpdclient 2.12
 */
bool
mpd_send_queue_changes_brief_range(struct mpd_connection *connection,
				   unsigned version,
				   unsigned start, unsigned end);

/**
 * Receives a response element of mpd_send_queue_changes_brief() or
 * mpd_send_queue_changes_brief_range().
 *
 * @param connection A valid and connected mpd_connection.
 * @param position_r reference to the position of the changed song
 * @param id_r reference to the id of the changed song
 * @return true on success, false on error or if there are no more
 * changes in this response
 */
bool
mpd_recv_queue_change_brief(struct mpd_connection *connection,
			    unsigned *position_r, unsigned *id_r);

/**
 * Appends a song to the playlist: either a single file or a directory.
 *
 * @param connection A valid and connected mpd_connection.
 * @param uri URI of a song or directory (added recursively)
 * @return true on success, false on error
 */
bool
mpd_send_add(struct mpd_connection *connection, const char *uri);

/**
 * Shortcut for mpd_send_add() and mpd_response_finish().
 *
 * @param connection the connection to MPD
 * @param uri URI of a song or directory (added recursively)
 * @return true on success, false on error
 */
bool
mpd_run_add(struct mpd_connection *connection, const char *uri);

/**
 * Inserts a song into the playlist for a given position: either a single file or a directory.
 *
 * @param connection A valid and connected mpd_connection.
 * @param uri URI of a song or directory (added recursively)
 * @param to the desired position of the song
 * @param whence how to interpret the position parameter
 * @return true on success, false on error
 *
 * @since libmpdclient 2.20
 */
bool
mpd_send_add_whence(struct mpd_connection *connection, const char *uri,
			unsigned to, enum mpd_position_whence whence);

/**
 * Shortcut for mpd_send_add_whence() and mpd_response_finish().
 *
 * @param connection the connection to MPD
 * @param uri URI of a song or directory (added recursively)
 * @param to the desired position of the song
 * @param whence how to interpret the position parameter
 * @return true on success, false on error
 *
 * @since libmpdclient 2.20
 */
bool
mpd_run_add_whence(struct mpd_connection *connection, const char *uri,
			unsigned to, enum mpd_position_whence whence);

/**
 * Appends a song to the playlist. Call mpd_recv_song_id() for its id.
 * file is always a single file or URL.
 *
 * @param connection the connection to MPD
 * @param uri URI of the song to be added
 * @return true on success, false on error
 */
bool
mpd_send_add_id(struct mpd_connection *connection, const char *uri);

/**
 * Inserts a song into the playlist for a given position, and returns its id.
 * file is always a single file or URL.
 *
 * @param connection the connection to MPD
 * @param uri the URI of the song to be added
 * @param to the desired position of the song
 * @return true on success, false on error
 */
bool
mpd_send_add_id_to(struct mpd_connection *connection, const char *uri,
		   unsigned to);

/**
 * Inserts a song into the playlist for a given position, and returns its id.
 * file is always a single file or URL.
 *
 * @param connection the connection to MPD
 * @param uri the URI of the song to be added
 * @param to the desired position of the song
 * @param whence how to interpret the position parameter
 * @return true on success, false on error
 * 
 * @since libmpdclient 2.20
 */
bool
mpd_send_add_id_whence(struct mpd_connection *connection, const char *uri,
		   unsigned to, enum mpd_position_whence whence);

/**
 * Returns the id of the new song in the playlist.  To be called after
 * mpd_send_add_id() or mpd_send_add_id_to().
 *
 * @param connection the connection to MPD
 * @return the new song id, -1 on error or if MPD did not send an id
 */
int
mpd_recv_song_id(struct mpd_connection *connection);

/**
 * Executes the "addid" command and reads the response.
 * file is always a single file or URL.
 *
 * @param connection the connection to MPD
 * @param uri URI of a song to be added
 * @return the new song id, -1 on error or if MPD did not send an id
 */
int
mpd_run_add_id(struct mpd_connection *connection, const char *uri);

/**
 * Executes the "addid" command and reads the response.
 * file is always a single file or URL.
 *
 * @param connection the connection to MPD
 * @param uri the URI of the song to be added
 * @param to the desired position of the song
 * @return the new song id, -1 on error or if MPD did not send an id
 */
int
mpd_run_add_id_to(struct mpd_connection *connection, const char *uri,
		  unsigned to);

/**
 * Executes the "addid" command and reads the response.
 * file is always a single file or URL.
 *
 * @param connection the connection to MPD
 * @param uri the URI of the song to be added
 * @param to the desired position of the song
 * @param whence how to interpret the position parameter
 * @return the new song id, -1 on error or if MPD did not send an id
 * 
 * @since libmpdclient 2.20
 */
int
mpd_run_add_id_whence(struct mpd_connection *connection, const char *uri,
		  unsigned to, enum mpd_position_whence whence);

/**
 * Deletes a song from the queue.
 *
 * @param connection the connection to MPD
 * @param pos the position of the song to be deleted
 * @return true on success, false on error
 */
bool
mpd_send_delete(struct mpd_connection *connection, unsigned pos);

/**
 * Shortcut for mpd_send_delete() and mpd_response_finish().
 *
 * @param connection the connection to MPD
 * @param pos the position of the song to be deleted
 * @return true on success, false on error
 */
bool
mpd_run_delete(struct mpd_connection *connection, unsigned pos);

/**
 * Deletes songs from the queue.
 *
 * @param connection the connection to MPD
 * @param start the start position of the range (including)
 * @param end the end position of the range (excluding); the special
 * value "UINT_MAX" makes the end of the range open
 * @return true on success, false on error
 *
 * @since libmpdclient 2.8 added support for "UINT_MAX"
 */
bool
mpd_send_delete_range(struct mpd_connection *connection,
		      unsigned start, unsigned end);

/**
 * Shortcut for mpd_send_delete_range() and mpd_response_finish().
 *
 * @param connection the connection to MPD
 * @param start the start position of the range (including)
 * @param end the end position of the range (excluding); the special
 * value "UINT_MAX" makes the end of the range open
 * @return true on success, false on error
 *
 * @since libmpdclient 2.8 added support for "UINT_MAX"
 */
bool
mpd_run_delete_range(struct mpd_connection *connection,
		      unsigned start, unsigned end);

/**
 * Deletes a song from the queue.
 *
 * @param connection the connection to MPD
 * @param id the id of the song to be deleted
 * @return true on success, false on error
 */
bool
mpd_send_delete_id(struct mpd_connection *connection, unsigned id);

/**
 * Shortcut for mpd_send_delete_id() and mpd_response_finish().
 *
 * @param connection the connection to MPD
 * @param id the id of the song to be deleted
 * @return true on success, false on error
 */
bool
mpd_run_delete_id(struct mpd_connection *connection, unsigned id);

/**
 * Shuffles the queue.
 *
 * @param connection the connection to MPD
 * @return true on success, false on error
 */
bool
mpd_send_shuffle(struct mpd_connection *connection);

/**
 * Shortcut for mpd_send_shuffle() and mpd_response_finish().
 *
 * @param connection the connection to MPD
 * @return true on success, false on error
 */
bool
mpd_run_shuffle(struct mpd_connection *connection);

/**
 * Shuffles a range within the queue.
 *
 * @param connection the connection to MPD
 * @param start the start position of the range (including)
 * @param end the end position of the range (excluding); the special
 * value "UINT_MAX" makes the end of the range open
 * @return true on success, false on error
 *
 * @since libmpdclient 2.8 added support for "UINT_MAX"
 */
bool
mpd_send_shuffle_range(struct mpd_connection *connection,
		       unsigned start, unsigned end);

/**
 * Shortcut for mpd_send_shuffle_range() and mpd_response_finish().
 *
 * @param connection the connection to MPD
 * @param start the start position of the range (including)
 * @param end the end position of the range (excluding); the special
 * value "UINT_MAX" makes the end of the range open
 * @return true on success, false on error
 *
 * @since libmpdclient 2.8 added support for "UINT_MAX"
 */
bool
mpd_run_shuffle_range(struct mpd_connection *connection,
		      unsigned start, unsigned end);

/**
 * Clear the queue.
 *
 * @param connection the connection to MPD
 * @return true on success, false on error
 */
bool
mpd_send_clear(struct mpd_connection *connection);

/**
 * Shortcut for mpd_send_clear() and mpd_response_finish().
 *
 * @param connection the connection to MPD
 * @return true on success, false on error
 */
bool
mpd_run_clear(struct mpd_connection *connection);

/**
 * Moves a song within the queue.
 *
 * @param connection the connection to MPD
 * @param from the source song position
 * @param to the new position of the song
 * @return true on success, false on error
 */
bool
mpd_send_move(struct mpd_connection *connection, unsigned from, unsigned to);

/**
 * Shortcut for mpd_send_move() and mpd_response_finish().
 *
 * @param connection the connection to MPD
 * @param from the source song position
 * @param to the new position of the song
 * @return true on success, false on error
 */
bool
mpd_run_move(struct mpd_connection *connection, unsigned from, unsigned to);

/**
 * Moves a song within the queue.
 *
 * @param connection the connection to MPD
 * @param from the source song position
 * @param to the new position of the song
 * @param whence how to interpret the to parameter
 * @return true on success, false on error
 *
 * @since libmpdclient 2.21, MPD 0.23
 */
bool
mpd_send_move_whence(struct mpd_connection *connection, unsigned from,
			 unsigned to, enum mpd_position_whence whence);

/**
 * Shortcut for mpd_send_move_whence() and mpd_response_finish().
 *
 * @param connection the connection to MPD
 * @param from the source song position
 * @param to the new position of the song
 * @param whence how to interpret the to parameter
 * @return true on success, false on error
 *
 * @since libmpdclient 2.21, MPD 0.23
 */
bool
mpd_run_move_whence(struct mpd_connection *connection, unsigned from,
			 unsigned to, enum mpd_position_whence whence);

/**
 * Moves a song within the queue.
 *
 * @param connection the connection to MPD
 * @param from the source song id
 * @param to the new position of the song (not an id!)
 * @return true on success, false on error
 */
bool
mpd_send_move_id(struct mpd_connection *connection, unsigned from, unsigned to);

/**
 * Shortcut for mpd_send_move_id() and mpd_response_finish().
 *
 * @param connection the connection to MPD
 * @param from the source song id
 * @param to the new position of the song (not an id!)
 * @return true on success, false on error
 */
bool
mpd_run_move_id(struct mpd_connection *connection, unsigned from, unsigned to);

/**
 * Moves a song within the queue.
 *
 * @param connection the connection to MPD
 * @param from the source song id
 * @param to the new position of the song (not an id!)
 * @param whence how to interpret the to parameter
 * @return true on success, false on error
 *
 * @since libmpdclient 2.21, MPD 0.23
 */
bool
mpd_send_move_id_whence(struct mpd_connection *connection, unsigned from,
			 unsigned to, enum mpd_position_whence whence);

/**
 * Shortcut for mpd_send_move_id_whence() and mpd_response_finish().
 *
 * @param connection the connection to MPD
 * @param from the source song id
 * @param to the new position of the song (not an id!)
 * @param whence how to interpret the to parameter
 * @return true on success, false on error
 *
 * @since libmpdclient 2.21, MPD 0.23
 */
bool
mpd_run_move_id_whence(struct mpd_connection *connection, unsigned from,
			 unsigned to, enum mpd_position_whence whence);

/**
 * Moves a range of songs within the queue.
 *
 * @param connection the connection to MPD
 * @param start the start position of the range (including)
 * @param end the end position of the range (excluding); the special
 * value "UINT_MAX" makes the end of the range open
 * @param to the new position of the song range
 * @return true on success, false on error
 *
 * @since libmpdclient 2.8 added support for "UINT_MAX"
 */
bool
mpd_send_move_range(struct mpd_connection *connection,
		    unsigned start, unsigned end, unsigned to);

/**
 * Shortcut for mpd_send_move_range() and mpd_response_finish().
 *
 * @param connection the connection to MPD
 * @param start the start position of the range (including)
 * @param end the end position of the range (excluding); the special
 * value "UINT_MAX" makes the end of the range open
 * @param to the new position of the song range
 * @return true on success, false on error
 *
 * @since libmpdclient 2.8 added support for "UINT_MAX"
 */
bool
mpd_run_move_range(struct mpd_connection *connection,
		    unsigned start, unsigned end, unsigned to);

/**
 * Moves a range of songs within the queue.
 *
 * @param connection the connection to MPD
 * @param start the start position of the range (including)
 * @param end the end position of the range (excluding); the special
 * value "UINT_MAX" makes the end of the range open
 * @param to the new position of the song range
 * @param whence how to interpret the to parameter
 * @return true on success, false on error
 *
 * @since libmpdclient 2.21, MPD 0.23
 */
bool
mpd_send_move_range_whence(struct mpd_connection *connection, unsigned start,
			 unsigned end, unsigned to, enum mpd_position_whence whence);

/**
 * Shortcut for mpd_send_move_range_whence() and mpd_response_finish().
 *
 * @param connection the connection to MPD
 * @param start the start position of the range (including)
 * @param end the end position of the range (excluding); the special
 * value "UINT_MAX" makes the end of the range open
 * @param to the new position of the song range
 * @param whence how to interpret the to parameter
 * @return true on success, false on error
 *
 * @since libmpdclient 2.21, MPD 0.23
 */
bool
mpd_run_move_range_whence(struct mpd_connection *connection, unsigned start,
			 unsigned end, unsigned to, enum mpd_position_whence whence);

/**
 * Swap the position of two songs in the queue.
 *
 * @param connection the connection to MPD
 * @param pos1 the position of one song
 * @param pos2 the position of the other song
 * @return true on success, false on error
 */
bool
mpd_send_swap(struct mpd_connection *connection, unsigned pos1, unsigned pos2);

/**
 * Shortcut for mpd_send_swap() and mpd_response_finish().
 *
 * @param connection the connection to MPD
 * @param pos1 the position of one song
 * @param pos2 the position of the other song
 * @return true on success, false on error
 */
bool
mpd_run_swap(struct mpd_connection *connection, unsigned pos1, unsigned pos2);

/**
 * Swap the position of two songs in the queue.
 *
 * @param connection the connection to MPD
 * @param id1 the id of one song
 * @param id2 the id of the other song
 * @return true on success, false on error
 */
bool
mpd_send_swap_id(struct mpd_connection *connection, unsigned id1, unsigned id2);

/**
 * Shortcut for mpd_send_swap_id() and mpd_response_finish().
 *
 * @param connection the connection to MPD
 * @param id1 the id of one song
 * @param id2 the id of the other song
 * @return true on success, false on error
 */
bool
mpd_run_swap_id(struct mpd_connection *connection, unsigned id1, unsigned id2);

/**
 * Adds a tag to the specified song (command "addtagid").
 *
 * @param connection the connection to MPD
 * @param id the id of the song
 * @param tag the tag to be added
 * @param value the tag value
 * @return true on success, false on error
 *
 * @since libmpdclient 2.12, MPD 0.19
 */
bool
mpd_send_add_tag_id(struct mpd_connection *connection, unsigned id,
		    enum mpd_tag_type tag, const char *value);

/**
 * Shortcut for mpd_send_add_tag_id() and mpd_response_finish().
 *
 * @param connection the connection to MPD
 * @param id the id of the song
 * @param tag the tag to be added
 * @param value the tag value
 * @return true on success, false on error
 *
 * @since libmpdclient 2.12, MPD 0.19
 */
bool
mpd_run_add_tag_id(struct mpd_connection *connection, unsigned id,
		   enum mpd_tag_type tag, const char *value);

/**
 * Remove a tag from the specified song (command "cleartagid").
 *
 * @param connection the connection to MPD
 * @param id the id of the song
 * @param tag the tag to be cleared
 * @return true on success, false on error
 *
 * @since libmpdclient 2.12, MPD 0.19
 */
bool
mpd_send_clear_tag_id(struct mpd_connection *connection, unsigned id,
		      enum mpd_tag_type tag);

/**
 * Shortcut for mpd_send_clear_tag_id() and mpd_response_finish().
 *
 * @param connection the connection to MPD
 * @param id the id of the song
 * @param tag the tag to be cleared
 * @return true on success, false on error
 *
 * @since libmpdclient 2.12, MPD 0.19
 */
bool
mpd_run_clear_tag_id(struct mpd_connection *connection, unsigned id,
		     enum mpd_tag_type tag);

/**
 * Remove all tags from the specified song (command "cleartagid").
 *
 * @param connection the connection to MPD
 * @param id the id of the song
 * @return true on success, false on error
 *
 * @since libmpdclient 2.12, MPD 0.19
 */
bool
mpd_send_clear_all_tags_id(struct mpd_connection *connection, unsigned id);

/**
 * Shortcut for mpd_send_clear_all_tags_id() and mpd_response_finish().
 *
 * @param connection the connection to MPD
 * @param id the id of the song
 * @return true on success, false on error
 *
 * @since libmpdclient 2.12, MPD 0.19
 */
bool
mpd_run_clear_all_tags_id(struct mpd_connection *connection, unsigned id);

/**
 * Change the priority of the specified song.
 *
 * @param connection the connection to MPD
 * @param priority a number between 0 and 255
 * @param position the position of the song
 * @return true on success, false on error
 *
 * @since libmpdclient 2.6
 */
bool
mpd_send_prio(struct mpd_connection *connection, unsigned priority,
	      unsigned position);

/**
 * Shortcut for mpd_send_prio() and mpd_response_finish().
 *
 * @param connection the connection to MPD
 * @param priority a number between 0 and 255
 * @param position the position of the song
 * @return true on success, false on error
 *
 * @since libmpdclient 2.6
 */
bool
mpd_run_prio(struct mpd_connection *connection, unsigned priority,
	     unsigned position);

/**
 * Change the priority of a song range.
 *
 * @param connection the connection to MPD
 * @param priority a number between 0 and 255
 * @param start the start position of the range (including)
 * @param end the end position of the range (excluding); the special
 * value "UINT_MAX" makes the end of the range open
 * @return true on success, false on error
 *
 * @since libmpdclient 2.6
 * @since libmpdclient 2.8 added support for "UINT_MAX"
 */
bool
mpd_send_prio_range(struct mpd_connection *connection, unsigned priority,
		    unsigned start, unsigned end);

/**
 * Shortcut for mpd_send_prio_range() and mpd_response_finish().
 *
 * @param connection the connection to MPD
 * @param priority a number between 0 and 255
 * @param start the start position of the range (including)
 * @param end the end position of the range (excluding); the special
 * value "UINT_MAX" makes the end of the range open
 * @return true on success, false on error
 *
 * @since libmpdclient 2.6
 * @since libmpdclient 2.8 added support for "UINT_MAX"
 */
bool
mpd_run_prio_range(struct mpd_connection *connection, unsigned priority,
		   unsigned start, unsigned end);

/**
 * Change the priority of the specified song.
 *
 * @param connection the connection to MPD
 * @param priority a number between 0 and 255
 * @param id the id of the song
 * @return true on success, false on error
 *
 * @since libmpdclient 2.6
 */
bool
mpd_send_prio_id(struct mpd_connection *connection, unsigned priority,
		 unsigned id);

/**
 * Shortcut for mpd_send_prio_id() and mpd_response_finish().
 *
 * @param connection the connection to MPD
 * @param priority a number between 0 and 255
 * @param id the id of the song
 * @return true on success, false on error
 *
 * @since libmpdclient 2.6
 */
bool
mpd_run_prio_id(struct mpd_connection *connection, unsigned priority,
		unsigned id);

/**
 * Specify the portion of a song that shall be played.
 * The song is identified by its id and cannot be the currently playing song.
 *
 * The start/end values are offsets in seconds (fractional seconds allowed);
 * both are optional.
 *
 * @param connection the connection to MPD
 * @param id the id of the song (cannot be the currently playing song)
 * @param start the offset in seconds for starting the song
 * @param end the offset in seconds for ending the song; a negative
 * value makes the end of the range open
 * @return true on success, false on error
 *
 * @since libmpdclient 2.19, MPD 0.20
 */
bool
mpd_send_range_id(struct mpd_connection *connection, unsigned id,
		  float start, float end);

/**
 *
 * Shortcut for mpd_send_range_id() and mpd_response_finish().
 *
 * @param connection the connection to MPD
 * @param id the id of the song (cannot be the currently playing song)
 * @param start the offset in seconds for starting the song
 * @param end the offset in seconds for ending the song; a negative
 * value makes the end of the range open
 * @return true on success, false on error
 *
 * @since libmpdclient 2.19, MPD 0.20
 */
bool
mpd_run_range_id(struct mpd_connection *connection, unsigned id,
		 float start, float end);
#ifdef __cplusplus
}
#endif

#endif
