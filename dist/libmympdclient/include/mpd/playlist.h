// SPDX-License-Identifier: BSD-3-Clause
// Copyright The Music Player Daemon Project

/*! \file
 * \brief MPD client library
 *
 * Manipulate stored playlists.
 *
 * Do not include this header directly.  Use mpd/client.h instead.
 */

#ifndef LIBMPDCLIENT_PLAYLIST_H
#define LIBMPDCLIENT_PLAYLIST_H

#include "compiler.h"
#include "position.h"

#include <stdbool.h>
#include <time.h>

struct mpd_pair;
struct mpd_connection;

/**
 * \struct mpd_playlist
 *
 * An opaque representation for a playlist stored in MPD's
 * playlist directory.  Use the functions provided by this header to
 * access the object's attributes.
 */
struct mpd_playlist;

/**
 * MPDs queue save modes.
 *
 * @since libmpdclient 2.21, MPD 0.24.
 */
enum mpd_queue_save_mode {
	/** Create new playlist */
	MPD_QUEUE_SAVE_MODE_CREATE = 0,

	/** Replace existing playlist */
	MPD_QUEUE_SAVE_MODE_REPLACE,

	/** Append to existing playlist */
	MPD_QUEUE_SAVE_MODE_APPEND,

	/** Unknown queue save mode */
	MPD_QUEUE_SAVE_MODE_UNKNOWN
};

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Free memory allocated by the #mpd_playlist object.
 */
void
mpd_playlist_free(struct mpd_playlist *playlist);

/**
 * Duplicates a #mpd_playlist object.
 *
 * @return the new object, or NULL on out of memory
 */
mpd_malloc
struct mpd_playlist *
mpd_playlist_dup(const struct mpd_playlist *playlist);

/**
 * Returns the path name of this playlist file.  It does not begin
 * with a slash.
 */
mpd_pure
const char *
mpd_playlist_get_path(const struct mpd_playlist *playlist);

/**
 * @return the POSIX UTC time stamp of the last modification, or 0 if
 * that is unknown
 */
mpd_pure
time_t
mpd_playlist_get_last_modified(const struct mpd_playlist *playlist);

/**
 * Begins parsing a new playlist.
 *
 * @param pair the first pair in this playlist (name must be
 * "playlist")
 * @return the new #mpd_playlist object, or NULL on error (out of
 * memory, or pair name is not "playlist")
 */
mpd_malloc
struct mpd_playlist *
mpd_playlist_begin(const struct mpd_pair *pair);

/**
 * Parses the pair, adding its information to the specified
 * #mpd_playlist object.
 *
 * @return true if the pair was parsed and added to the playlist (or if
 * the pair was not understood and ignored), false if this pair is the
 * beginning of the next playlist
 */
bool
mpd_playlist_feed(struct mpd_playlist *playlist, const struct mpd_pair *pair);

/**
 * Obtain a list of stored playlists.
 * Use mpd_recv_playlist() to read the playlists.
 *
 * @param connection the connection to MPD
 * @return true on success, false on error
 *
 * @since libmpdclient 2.5
 */
bool
mpd_send_list_playlists(struct mpd_connection *connection);

/**
 * Receives the next playlist from the MPD server.
 *
 * @return a #mpd_playlist object, or NULL on error or if the playlist list is
 * finished
 */
mpd_malloc
struct mpd_playlist *
mpd_recv_playlist(struct mpd_connection *connection);

/**
 * List the content of the stored playlist identified by name.
 * Use mpd_recv_entity() to receive the songs (#MPD_ENTITY_TYPE_SONG).
 *
 * @param connection the connection to MPD
 * @param name the name of the playlist
 * @return true on success, false on error
 */
bool
mpd_send_list_playlist(struct mpd_connection *connection, const char *name);

/**
 * Like mpd_send_list_playlist(), but specifies a (position) range.
 * Use mpd_recv_entity() to receive the songs (#MPD_ENTITY_TYPE_SONG).
 *
 * @param connection the connection to MPD
 * @param name the name of the playlist
 * @param start the start position of the range (including)
 * @param end the end position of the range (excluding); the special
 * @return true on success, false on error
 *
 * @since libmpdclient 2.23, MPD 0.24
 */
bool
mpd_send_list_playlist_range(struct mpd_connection *connection, const char *name,
			     unsigned start, unsigned end);

/**
 * List the content, with full metadata, of the stored playlist identified by
 * name.  Use mpd_recv_entity() to receive the songs (#MPD_ENTITY_TYPE_SONG).
 *
 * @param connection the connection to MPD
 * @param name the name of the playlist
 * @return true on success, false on error
 */
bool
mpd_send_list_playlist_meta(struct mpd_connection *connection, const char *name);

/**
 * Like mpd_send_list_playlist_meta(), but specifies a (position) range.
 * Use mpd_recv_entity() to receive the songs (#MPD_ENTITY_TYPE_SONG).
 *
 * @param connection the connection to MPD
 * @param name the name of the playlist
 * @param start the start position of the range (including)
 * @param end the end position of the range (excluding); the special
 * @return true on success, false on error
 *
 * @since libmpdclient 2.23, MPD 0.24
 */
bool
mpd_send_list_playlist_range_meta(struct mpd_connection *connection, const char *name,
				  unsigned start, unsigned end);

/**
 * Clear the playlist name (i.e. truncate name.m3u)
 *
 * @param connection the connection to MPD
 * @param name the name of the playlist
 * @return true on success, false on error
 */
bool
mpd_send_playlist_clear(struct mpd_connection *connection, const char *name);

/**
 * Shortcut for mpd_send_playlist_clear() and mpd_response_finish().
 *
 * @param connection the connection to MPD
 * @param name the name of the playlist
 * @return true on success, false on error
 */
bool
mpd_run_playlist_clear(struct mpd_connection *connection, const char *name);

/**
 * Add a path to a playlist. The playlist will be created if it does not
 * exist.
 *
 * @param connection the connection to MPD
 * @param name the name of the playlist
 * @param path URI to be added
 * @return true on success, false on error
 */
bool
mpd_send_playlist_add(struct mpd_connection *connection, const char *name,
		      const char *path);

/**
 * Shortcut for mpd_send_playlist_add() and mpd_response_finish().
 *
 * @param connection the connection to MPD
 * @param name the name of the playlist
 * @param path URI to be added
 * @return true on success, false on error
 */
bool
mpd_run_playlist_add(struct mpd_connection *connection,
		     const char *name, const char *path);

/**
 * Inserts a uri to a playlist for a given position. The playlist 
 * will be created if it does not exist.
 *
 * @param connection the connection to MPD
 * @param name the name of the playlist
 * @param uri URI to be added
 * @param to the desired position
 * @return true on success, false on error
 *
 * @since libmpdclient 2.20
 */
bool
mpd_send_playlist_add_to(struct mpd_connection *connection, const char *name,
		      const char *uri, unsigned to);

/**
 * Shortcut for mpd_send_playlist_add_to() and mpd_response_finish().
 *
 * @param connection the connection to MPD
 * @param name the name of the playlist
 * @param uri URI to be added
 * @param to the desired position
 * @return true on success, false on error
 *
 * @since libmpdclient 2.20
 */
bool
mpd_run_playlist_add_to(struct mpd_connection *connection,
		     const char *name, const char *uri, unsigned to);

/**
 * Move a song from one position to another in the same playlist.
 *
 * @param connection the connection to MPD
 * @param name the name of the playlist
 * @param from previous song place in the playlist
 * @param to next song position in the playlist
 * @return true on success, false on error
 */
bool
mpd_send_playlist_move(struct mpd_connection *connection, const char *name,
		       unsigned from, unsigned to);

/**
 * Shortcut for mpd_send_playlist_move() and mpd_response_finish().
 *
 * @param connection the connection to MPD
 * @param name the name of the playlist
 * @param from previous song place in the playlist
 * @param to next song position in the playlist
 * @return true on success, false on error
 *
 * @since libmpdclient 2.19
 */
bool
mpd_run_playlist_move(struct mpd_connection *connection, const char *name,
		      unsigned from, unsigned to);

/**
 * Moves a range of songs within the same playlist.
 *
 * @param connection the connection to MPD
 * @param name the name of the playlist
 * @param start the start position of the range (including)
 * @param end the end position of the range (excluding); the special
 * value "UINT_MAX" makes the end of the range open
 * @param to the new position of the song range
 * @return true on success, false on error
 *
 * @since libmpdclient 2.21, MPD 0.24
 */
bool
mpd_send_playlist_move_range(struct mpd_connection *connection, const char *name,
		       unsigned start, unsigned end, unsigned to);

/**
 * Shortcut for mpd_send_playlist_move_range() and mpd_response_finish().
 *
 * @param connection the connection to MPD
 * @param name the name of the playlist
 * @param start the start position of the range (including)
 * @param end the end position of the range (excluding); the special
 * value "UINT_MAX" makes the end of the range open
 * @param to the new position of the song range
 * @return true on success, false on error
 *
 * @since libmpdclient 2.21, MPD 0.24
 */
bool
mpd_run_playlist_move_range(struct mpd_connection *connection, const char *name,
		       unsigned start, unsigned end, unsigned to);

/**
 * Delete a position from a playlist.
 *
 * @param connection the connection to MPD
 * @param name the name of the playlist
 * @param pos song position in the playlist
 * @return true on success, false on error
 */
bool
mpd_send_playlist_delete(struct mpd_connection *connection, const char *name,
			 unsigned pos);

/**
 * Shortcut for mpd_send_playlist_delete() and mpd_response_finish().
 *
 * @param connection the connection to MPD
 * @param name the name of the playlist
 * @param pos song position in the playlist
 * @return true on success, false on error
 */
bool
mpd_run_playlist_delete(struct mpd_connection *connection,
			const char *name, unsigned pos);

/**
 * Delete a range from a playlist.
 *
 * @param connection the connection to MPD
 * @param name the name of the playlist
 * @param start the start position of the range (including)
 * @param end the end position of the range (excluding)
 * @return true on success, false on error
 *
 * @since libmpdclient 2.20
 */
bool
mpd_send_playlist_delete_range(struct mpd_connection *connection, const char *name,
			 unsigned start, unsigned end);

/**
 * Shortcut for mpd_send_playlist_delete_range() and mpd_response_finish().
 *
 * @param connection the connection to MPD
 * @param name the name of the playlist
 * @param start the start position of the range (including)
 * @param end the end position of the range (excluding)
 * @return true on success, false on error
 *
 * @since libmpdclient 2.20
 */
bool
mpd_run_playlist_delete_range(struct mpd_connection *connection,
			const char *name, unsigned start, unsigned end);

/**
 * Saves the current queue as a m3u file in the playlist directory
 * (i.e. name.m3u).
 *
 * @param connection the connection to MPD
 * @param name the name of the playlist file
 * @return true on success, false on error
 */
bool
mpd_send_save(struct mpd_connection *connection, const char *name);

/**
 * Shortcut for mpd_send_save() and mpd_response_finish().
 *
 * @param connection the connection to MPD
 * @param name the name of the playlist file
 * @return true on success, false on error
 */
bool
mpd_run_save(struct mpd_connection *connection, const char *name);

/**
 * Saves the current queue as a m3u file in the playlist directory
 * (i.e. name.m3u).
 * This function supports the mode argument that can be
 * MPD_QUEUE_SAVE_MODE_CREATE: creates a new playlist, same as mpd_send_save
 * MPD_QUEUE_SAVE_MODE_REPLACE: replaces an existing playlist
 * MPD_QUEUE_SAVE_MODE_APPEND: appends to an existing playlist
 *
 * @since libmpdclient 2.21, MPD 0.24.
 *
 * @param connection the connection to MPD
 * @param name the name of the playlist file
 * @param mode the desired save mode
 * @return true on success, false on error
 */
bool
mpd_send_save_queue(struct mpd_connection *connection, const char *name,
			enum mpd_queue_save_mode mode);

/**
 * Shortcut for mpd_send_save_queue() and mpd_response_finish().
 *
 * @since libmpdclient 2.21, MPD 0.24.
 *
 * @param connection the connection to MPD
 * @param name the name of the playlist file
 * @param mode the desired save mode
 * @return true on success, false on error
 */
bool
mpd_run_save_queue(struct mpd_connection *connection, const char *name,
			enum mpd_queue_save_mode mode);

/**
 * Looks up the name of the specified queue save mode.
 *
 * @return the name, or NULL if the queue save mode is not valid
 *
 * @since libmpdclient 2.21.
 */
const char *
mpd_lookup_queue_save_mode(enum mpd_queue_save_mode mode);

/**
 * Parse the string to check which queue save mode it contains.
 *
 * @return the queue save mode enum
 *
 * @since libmpdclient 2.21.
 */
enum mpd_queue_save_mode
mpd_parse_queue_save_mode(const char *mode);

/**
 * Load a stored playlist into the queue.
 *
 * @param connection the connection to MPD
 * @param name the name of the playlist file
 * @return true on success, false on error
 *
 * @since libmpdclient 2.0
 */
bool
mpd_send_load(struct mpd_connection *connection, const char *name);

/**
 * Shortcut for mpd_send_load() and mpd_response_finish().
 *
 * @param connection the connection to MPD
 * @param name the name of the playlist file
 * @return true on success, false on error
 *
 * @since libmpdclient 2.0
 */
bool
mpd_run_load(struct mpd_connection *connection, const char *name);

/**
 * Like mpd_send_load(), but load only a portion of the playlist.
 *
 * @param connection the connection to MPD
 * @param name the name of the playlist file
 * @param start the start position of the range (including)
 * @param end the end position of the range (excluding); the special
 * value "UINT_MAX" makes the end of the range open
 * @return true on success, false on error
 *
 * @since libmpdclient 2.16
 */
bool
mpd_send_load_range(struct mpd_connection *connection, const char *name,
		    unsigned start, unsigned end);

/**
 * Shortcut for mpd_send_load_range() and mpd_response_finish().
 *
 * @param connection the connection to MPD
 * @param name the name of the playlist file
 * @param start the start position of the range (including)
 * @param end the end position of the range (excluding); the special
 * value "UINT_MAX" makes the end of the range open
 * @return true on success, false on error
 *
 * @since libmpdclient 2.16
 */
bool
mpd_run_load_range(struct mpd_connection *connection, const char *name,
		   unsigned start, unsigned end);

/**
 * Like mpd_send_load() or mpd_send_load_range(), but with range 
 * and to parameters.
 *
 * @param connection the connection to MPD
 * @param name the name of the playlist file
 * @param start the start position of the range (including)
 * @param end the end position of the range (excluding); the special
 * value "UINT_MAX" makes the end of the range open
 * @param to the desired position of the song
 * @param whence how to interpret the position parameter
 * @return true on success, false on error
 *
 * @since libmpdclient 2.20
 */
bool
mpd_send_load_range_to(struct mpd_connection *connection, const char *name,
		   unsigned start, unsigned end, unsigned to, enum mpd_position_whence whence);

/**
 * Shortcut for mpd_send_load_range_to() and mpd_response_finish().
 *
 * @param connection the connection to MPD
 * @param name the name of the playlist file
 * @param start the start position of the range (including)
 * @param end the end position of the range (excluding); the special
 * value "UINT_MAX" makes the end of the range open
 * @param to the desired position of the song
 * @param whence how to interpret the position parameter
 * @return true on success, false on error
 *
 * @since libmpdclient 2.20
 */
bool
mpd_run_load_range_to(struct mpd_connection *connection, const char *name,
		   unsigned start, unsigned end, unsigned to, enum mpd_position_whence whence);

/**
 * Rename a playlist in the playlist directory.
 *
 * @param connection the connection to MPD
 * @param from the previous name of the playlist file
 * @param to the next name of the playlist file
 * @return true on success, false on error
 */
bool
mpd_send_rename(struct mpd_connection *connection,
		const char *from, const char *to);

/**
 * Shortcut for mpd_send_rename() and mpd_response_finish().
 *
 * @param connection the connection to MPD
 * @param from the previous name of the playlist file
 * @param to the next name of the playlist file
 * @return true on success, false on error
 */
bool
mpd_run_rename(struct mpd_connection *connection,
	       const char *from, const char *to);

/**
 * Remove a playlist from the playlist directory.
 *
 * @param connection the connection to MPD
 * @param name the name of the playlist file
 * @return true on success, false on error
 */
bool
mpd_send_rm(struct mpd_connection *connection, const char *name);

/**
 * Shortcut for mpd_send_rm() and mpd_response_finish().
 *
 * @param connection the connection to MPD
 * @param name the name of the playlist file
 * @return true on success, false on error
 */
bool
mpd_run_rm(struct mpd_connection *connection, const char *name);

/**
 * Count the number of songs and their total playtime (seconds) in the
 * playlist.
 *
 * @param connection the connection to MPD
 * @param name the name of the playlist file
 * @return true on success, false on error
 *
 * @since libmpdclient 2.23, MPD 0.24
 */
bool
mpd_send_playlistlength(struct mpd_connection *connection, const char *name);

#ifdef __cplusplus
}
#endif

#endif
