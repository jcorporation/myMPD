// SPDX-License-Identifier: BSD-3-Clause
// Copyright The Music Player Daemon Project

/*! \file
 * \brief MPD client library
 *
 * Do not include this header directly.  Use mpd/client.h instead.
 */

#ifndef MPD_STATUS_H
#define MPD_STATUS_H

#include "compiler.h"

#include <stdbool.h>

/**
 * MPD's playback state.
 */
enum mpd_state {
	/** no information available */
	MPD_STATE_UNKNOWN = 0,

	/** not playing */
	MPD_STATE_STOP = 1,

	/** playing */
	MPD_STATE_PLAY = 2,

	/** playing, but paused */
	MPD_STATE_PAUSE = 3,
};

/**
 * MPD's single state.
 *
 * @since libmpdclient 2.18, MPD 0.21.
 */
enum mpd_single_state {
	/** disabled */
	MPD_SINGLE_OFF = 0,

	/** enabled */
	MPD_SINGLE_ON,

	/**
	 * enables single state (#MPD_SINGLE_ONESHOT) for a single song, then
	 * MPD disables single state (#MPD_SINGLE_OFF) if the current song
	 * has played and there is another song in the current playlist
	 *
	 * @since MPD 0.21
	 **/
	MPD_SINGLE_ONESHOT,

	/** Unknown state */
	MPD_SINGLE_UNKNOWN,
};

/**
 * MPD's consume state.
 *
 * @since libmpdclient 2.21, MPD 0.24.
 */
enum mpd_consume_state {
	/** disabled */
	MPD_CONSUME_OFF = 0,

	/** enabled */
	MPD_CONSUME_ON,

	/**
	 * enables consume state (#MPD_CONSUME_ONESHOT) for a single song, then
	 * MPD disables consume state (#MPD_CONSUME_OFF) if the current song
	 * has played.
	 *
	 * @since MPD 0.24
	 **/
	MPD_CONSUME_ONESHOT,

	/** Unknown state */
	MPD_CONSUME_UNKNOWN,
};

struct mpd_connection;
struct mpd_pair;
struct mpd_audio_format;

/**
 * \struct mpd_status
 *
 * Holds information about MPD's status.
 */
struct mpd_status;

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Begins parsing the server status: creates a new empty #mpd_status
 * object.  Free it with mpd_status_free().
 *
 * @return the newly allocated #mpd_status object, or NULL if out of
 * memory
 */
mpd_malloc
struct mpd_status *
mpd_status_begin(void);

/**
 * Parses the pair, adding its information to the specified
 * #mpd_status object.
 */
void
mpd_status_feed(struct mpd_status *status, const struct mpd_pair *pair);

/**
 * Sends the "status" command to MPD.  Call mpd_recv_status() to read
 * the response.
 *
 * @return true on success
 */
bool
mpd_send_status(struct mpd_connection *connection);

/**
 * Receives a #mpd_status object from the server.
 *
 * @return the received #mpd_status object, or NULL on error
 */
mpd_malloc
struct mpd_status *
mpd_recv_status(struct mpd_connection *connection);

/**
 * Executes the "status" command and reads the response.
 *
 * @return the #mpd_status object returned by the server, or NULL on
 * error
 */
mpd_malloc
struct mpd_status *
mpd_run_status(struct mpd_connection *connection);

/**
 * Releases a #mpd_status object.
 */
void mpd_status_free(struct mpd_status * status);

/**
 * Returns the current volume: 0-100, or -1 when there is no volume
 * support.
 */
mpd_pure
int mpd_status_get_volume(const struct mpd_status *status);

/**
 * Returns true if repeat mode is on.
 */
mpd_pure
bool
mpd_status_get_repeat(const struct mpd_status *status);

/**
 * Returns true if random mode is on.
 */
mpd_pure
bool
mpd_status_get_random(const struct mpd_status *status);

/**
 * Returns the current state of single mode on MPD.
 *
 * If the state is #MPD_SINGLE_ONESHOT, MPD will transition to #MPD_SINGLE_OFF
 * after a song is played and if there is another song in the queue. The
 * #mpd_status object will not be updated accordingly. In this case, you need
 * to call mpd_send_status() and mpd_recv_status() again.
 *
 * @since MPD 0.21, libmpdclient 2.18.
 */
mpd_pure
enum mpd_single_state
mpd_status_get_single_state(const struct mpd_status *status);

/**
 * Looks up the name of the specified single mode.
 *
 * @return the name, or NULL if the single mode is not valid
 *
 * @since libmpdclient 2.21.
 */
const char *
mpd_lookup_single_state(enum mpd_single_state state);

/**
 * Parse the string to check which single mode it contains.
 *
 * @return the single mode enum
 *
 * @since libmpdclient 2.21.
 */
enum mpd_single_state
mpd_parse_single_state(const char *p);

/**
 * This function is deprecated as it does not distinguish the states of
 * the single mode (added to MPD 0.21). Call mpd_status_get_single_state() in
 * its place.
 *
 * Returns true if single mode is either on or in oneshot.
 */
mpd_pure
bool
mpd_status_get_single(const struct mpd_status *status);

/**
 * Returns the current state of consume mode on MPD.
 *
 * If the state is #MPD_CONSUME_ONESHOT, MPD will transition to #MPD_CONSUME_OFF
 * after a song is played. The #mpd_status object will not be updated accordingly.
 * In this case, you need to call mpd_send_status() and mpd_recv_status() again.
 *
 * @since MPD 0.24, libmpdclient 2.21.
 */
mpd_pure
enum mpd_consume_state
mpd_status_get_consume_state(const struct mpd_status *status);

/**
 * Looks up the name of the specified consume mode.
 *
 * @return the name, or NULL if the consume mode is not valid
 *
 * @since libmpdclient 2.21.
 */
const char *
mpd_lookup_consume_state(enum mpd_consume_state state);

/**
 * Parse the string to check which consume mode it contains.
 *
 * @return the consume mode enum
 *
 * @since libmpdclient 2.21.
 */
enum mpd_consume_state
mpd_parse_consume_state(const char *p);

/**
 * This function is deprecated as it does not distinguish the states of
 * the consume mode (added to MPD 0.24). Call mpd_status_get_consume_state() in
 * its place.
 *
 * Returns true if consume mode is either on or in oneshot.
 */
mpd_pure
bool
mpd_status_get_consume(const struct mpd_status *status);

/**
 * Returns the number of songs in the queue.  If MPD did not
 * specify that, this function returns 0.
 */
mpd_pure
unsigned
mpd_status_get_queue_length(const struct mpd_status *status);

/**
 * Returns queue version number.  You may use this to determine
 * when the queue has changed since you have last queried it.
 */
mpd_pure
unsigned
mpd_status_get_queue_version(const struct mpd_status *status);

/**
 * Returns the state of the player: either stopped, playing or paused.
 */
mpd_pure
enum mpd_state
mpd_status_get_state(const struct mpd_status *status);

/**
 * Returns crossfade setting in seconds.  0 means crossfading is
 * disabled.
 */
mpd_pure
unsigned
mpd_status_get_crossfade(const struct mpd_status *status);

/**
 * Returns mixrampdb setting in db. 0 means mixrampdb is disabled.
 *
 * @since libmpdclient 2.2
 */
mpd_pure
float
mpd_status_get_mixrampdb(const struct mpd_status *status);

/**
 * Returns mixrampdelay setting in seconds.  Negative means mixrampdelay is
 * disabled.
 *
 * @since libmpdclient 2.2
 */
mpd_pure
float
mpd_status_get_mixrampdelay(const struct mpd_status *status);

/**
 * Returns the position of the currently playing song in the queue
 * (beginning with 0) if a song is currently selected (always the case when
 * state is MPD_STATE_PLAY or MPD_STATE_PAUSE).  If there is no current song,
 * -1 is returned.
 */
mpd_pure
int
mpd_status_get_song_pos(const struct mpd_status *status);

/**
 * Returns the id of the current song.  If there is no current song,
 * -1 is returned.
 */
mpd_pure
int
mpd_status_get_song_id(const struct mpd_status *status);

/**
 * The same as mpd_status_get_song_pos(), but for the next song to be
 * played.
 *
 * @since libmpdclient 2.7
 */
mpd_pure
int
mpd_status_get_next_song_pos(const struct mpd_status *status);

/**
 * Returns the id of the next song to be played.  If it is not known, -1 is
 * returned.
 *
 * @since libmpdclient 2.7
 */
mpd_pure
int
mpd_status_get_next_song_id(const struct mpd_status *status);

/**
 * This function uses a deprecated feature of MPD, call
 * mpd_status_get_elapsed_ms() instead.
 *
 * Returns time in seconds that have elapsed in the currently playing/paused
 * song.
 *
 */
mpd_pure
unsigned
mpd_status_get_elapsed_time(const struct mpd_status *status);

/**
 * Returns time in milliseconds that have elapsed in the currently
 * playing/paused song.
 *
 * @since libmpdclient 2.1
 */
mpd_pure
unsigned
mpd_status_get_elapsed_ms(const struct mpd_status *status);

/**
 * Returns the length in seconds of the currently playing/paused song
 */
mpd_pure
unsigned
mpd_status_get_total_time(const struct mpd_status *status);

/**
 * Returns current bit rate in kbps.  0 means unknown.
 */
mpd_pure
unsigned
mpd_status_get_kbit_rate(const struct mpd_status *status);

/**
 * Returns audio format which MPD is currently playing.  May return
 * NULL if MPD is not playing or if the audio format is unknown.
 */
mpd_pure
const struct mpd_audio_format *
mpd_status_get_audio_format(const struct mpd_status *status);

/**
 * Returns 1 if mpd is updating, 0 otherwise
 */
mpd_pure
unsigned
mpd_status_get_update_id(const struct mpd_status *status);

/**
 * Returns the name of the current partition or NULL if the server did
 * not send a name.
 */
mpd_pure
const char *
mpd_status_get_partition(const struct mpd_status *status);

/**
 * Returns the error message
 */
mpd_pure
const char *
mpd_status_get_error(const struct mpd_status *status);

#ifdef __cplusplus
}
#endif

#endif
