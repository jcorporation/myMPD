/* libmpdclient
   (c) 2003-2019 The Music Player Daemon Project
   This project's homepage is: http://www.musicpd.org

   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions
   are met:

   - Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.

   - Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.

   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
   ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
   A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR
   CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
   EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
   PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
   PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
   LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
   NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
   SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

/*! \file
 * \brief MPD client library
 *
 * Manipulate stickers.
 *
 * Do not include this header directly.  Use mpd/client.h instead.
 */

#ifndef MPD_STICKER_H
#define MPD_STICKER_H

#include "compiler.h"

#include <stdbool.h>
#include <stddef.h>

struct mpd_connection;

/**
 * Comparison operators for sticker search api
 */
enum mpd_sticker_operator {
	MPD_STICKER_OP_UNKOWN = -1,
	MPD_STICKER_OP_EQ,
	MPD_STICKER_OP_GT,
	MPD_STICKER_OP_LT,
	MPD_STICKER_OP_GT_INT,
	MPD_STICKER_OP_LT_INT,
};

/**
 * Comparison operators for sticker search api
 */
enum mpd_sticker_sort {
	MPD_STICKER_SORT_UNKOWN = -1,
	MPD_STICKER_SORT_URI,
	MPD_STICKER_SORT_VALUE,
	MPD_STICKER_SORT_VALUE_INT,
};

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Stickers are pieces of information attached to existing MPD objects (e.g.
 * song files, directories, albums; but currently, they are only implemented
 * for song). Clients can create arbitrary name/value pairs. MPD itself does
 * not assume any special meaning in them.
 *
 * The goal is to allow clients to share additional (possibly dynamic)
 * information about songs, which is neither stored on the client (not
 * available to other clients), nor stored in the song files (MPD has no write
 * access).
 *
 * Client developers should create a standard for common sticker names, to
 * ensure interoperability.
 *
 * Objects which may have stickers are addressed by their object type ("song"
 * for song objects) and their URI (the path within the database for songs).
 */

/**
 * Adds or replaces a sticker value.
 *
 * @param connection the connection to MPD
 * @param type the object type, e.g. "song"
 * @param uri the URI of the object
 * @param name the name of the sticker
 * @param value the value of the sticker
 * @return true on success, false on error
 *
 * @since libmpdclient 2.1
 */
bool
mpd_send_sticker_set(struct mpd_connection *connection, const char *type,
		     const char *uri, const char *name, const char *value);

/**
 * Shortcut for mpd_send_sticker_set() and mpd_response_finish().
 *
 * @param connection the connection to MPD
 * @param type the object type, e.g. "song"
 * @param uri the URI of the object
 * @param name the name of the sticker
 * @param value the value of the sticker
 * @return true on success, false on error
 *
 * @since libmpdclient 2.1
 */
bool
mpd_run_sticker_set(struct mpd_connection *connection, const char *type,
		    const char *uri, const char *name, const char *value);

/**
 * Deletes a sticker value.
 *
 * @param connection the connection to MPD
 * @param type the object type, e.g. "song"
 * @param uri the URI of the object
 * @param name the name of the sticker
 * @return true on success, false on error
 *
 * @since libmpdclient 2.1
 */
bool
mpd_send_sticker_delete(struct mpd_connection *connection, const char *type,
			const char *uri, const char *name);

/**
 * Shortcut for mpd_send_sticker_delete() and mpd_response_finish().
 *
 * @param connection the connection to MPD
 * @param type the object type, e.g. "song"
 * @param uri the URI of the object
 * @param name the name of the sticker
 * @return true on success, false on error
 *
 * @since libmpdclient 2.1
 */
bool
mpd_run_sticker_delete(struct mpd_connection *connection, const char *type,
		       const char *uri, const char *name);

/**
 * Queries a sticker value.  Call mpd_recv_sticker() to receive the response.
 *
 * @param connection the connection to MPD
 * @param type the object type, e.g. "song"
 * @param uri the URI of the object
 * @param name the name of the sticker
 * @return true on success, false on error
 *
 * @since libmpdclient 2.1
 */
bool
mpd_send_sticker_get(struct mpd_connection *connection, const char *type,
		     const char *uri, const char *name);

/**
 * Obtains a list of all stickers of the specified object.  Call
 * mpd_recv_sticker() to receive each response item.
 *
 * @param connection the connection to MPD
 * @param type the object type, e.g. "song"
 * @param uri the URI of the object
 * @return true on success, false on error
 *
 * @since libmpdclient 2.1
 */
bool
mpd_send_sticker_list(struct mpd_connection *connection, const char *type,
		      const char *uri);

/**
 * Searches for stickers with the specified name. Call mpd_recv_sticker() to
 * receive each response item.
 *
 * @param connection the connection to MPD
 * @param type the object type, e.g. "song"
 * @param base_uri the base URI to start the search, e.g. a directory;
 * NULL to search for all objects of the specified type
 * @param name the name of the sticker
 * @return true on success, false on error
 *
 * @since libmpdclient 2.1
 */
bool
mpd_send_sticker_find(struct mpd_connection *connection, const char *type,
		      const char *base_uri, const char *name);

/**
 * Parse a sticker input line in the form "name=value".
 *
 * @param input the input value, the value from a received pair named
 * "sticker"
 * @param name_length_r the length of the name (starting at the
 * beginning of the input string) is returned here
 * @return a pointer to the sticker value, or NULL on error
 *
 * @since libmpdclient 2.1
 */
const char *
mpd_parse_sticker(const char *input, size_t *name_length_r);

/**
 * Receives the next sticker.  You have to free the return value with
 * mpd_return_sticker().
 *
 * @param connection the connection to MPD
 * @return a #mpd_pair object on success, NULL on end of response or
 * error
 *
 * @since libmpdclient 2.1
 */
mpd_malloc
struct mpd_pair *
mpd_recv_sticker(struct mpd_connection *connection);

/**
 * Free the pair returned by mpd_recv_sticker().
 *
 * @since libmpdclient 2.1
 */
void
mpd_return_sticker(struct mpd_connection *connection, struct mpd_pair *pair);

/**
 * Obtains an uniq and sortes list of all sticker names. Call
 * mpd_recv_pair() to receive each response item.
 *
 * @param connection the connection to MPD
 * @return true on success, false on error
 *
 * @since libmpdclient 2.21, MPD 0.24
 */
bool
mpd_send_stickernames(struct mpd_connection *connection);

/**
 * Search for stickers in the database.
 * Constraints may be specified with mpd_search_add_tag_constraint().
 * Send the search command with mpd_search_commit(), and read the
 * response items with mpd_recv_song().
 *
 * @param connection the connection to MPD
 * @param type the object type, e.g. "song"
 * @param base_uri the base URI to start the search, e.g. a directory;
 * NULL to search for all objects of the specified type
 * @param name the name of the sticker
 * @return true on success, false on error
 *
 * @since libmpdclient 2.21, MPD 0.24
 */
bool
mpd_sticker_search_begin(struct mpd_connection *connection, const char *type,
			 const char *base_uri, const char *name);

/**
 * Adds the value constraint to the search
 * @param connection a #mpd_connection
 * @param oper compare operator
 * @param value value to compare against
 * @return true on success, else false
 *
 * @since libmpdclient 2.21, MPD 0.24
 */
bool
mpd_sticker_search_add_value_constraint(struct mpd_connection *connection,
					enum mpd_sticker_operator oper,
					const char *value);

/**
 * Sort the results by the specified named attribute.
 *
 * @param connection a #mpd_connection
 * @param sort sort operator
 * @param descending sort in reverse order?
 * @return true on success, false on error
 *
 * @since libmpdclient 2.21, MPD 0.24
 */
bool
mpd_sticker_search_add_sort(struct mpd_connection *connection,
			    enum mpd_sticker_sort sort, bool descending);

/**
 * Request only a portion of the result set.
 *
 * @param connection a #mpd_connection
 * @param start the start offset (including)
 * @param end the end offset (not including)
 * value "UINT_MAX" makes the end of the range open
 * @return true on success, false on error
 *
 * @since libmpdclient 2.21, MPD 0.24
 */
bool
mpd_sticker_search_add_window(struct mpd_connection *connection,
		              unsigned start, unsigned end);

/**
 * Starts the real search with constraints added with
 * mpd_sticker_search_add_constraint().
 *
 * @param connection the connection to MPD
 * @return true on success, false on error
 *
 * @since libmpdclient 2.21, MPD 0.24
 */
bool
mpd_sticker_search_commit(struct mpd_connection *connection);

/**
 * Cancels the search request before you have called
 * mpd_sticker_search_commit(). Call this to clear the current search
 * request.
 *
 * @param connection the connection to MPD
 *
 * @since libmpdclient 2.21, MPD 0.24
 */
void
mpd_sticker_search_cancel(struct mpd_connection *connection);

#ifdef __cplusplus
}
#endif

#endif /* MPD_STICKER_H */

