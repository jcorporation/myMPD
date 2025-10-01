// SPDX-License-Identifier: BSD-3-Clause
// Copyright The Music Player Daemon Project

#include <mpd/status.h>
#include <mpd/pair.h>
#include <mpd/audio_format.h>
#include "iaf.h"

#include <assert.h>
#include <inttypes.h>
#include <stdlib.h>
#include <string.h>

/**
 * Information about MPD's current status.
 */
struct mpd_status {
	/** 0-100, or MPD_STATUS_NO_VOLUME when there is no volume support */
	int volume;

	/** Queue repeat mode enabled? */
	bool repeat;

	/** Random mode enabled? */
	bool random;

	/** Single song mode enabled? */
	enum mpd_single_state single;

	/** Song consume mode enabled? */
	enum mpd_consume_state consume;

	/** Number of songs in the queue */
	unsigned queue_length;

	/**
	 * Queue version, use this to determine when the playlist has
	 * changed.
	 */
	unsigned queue_version;

	/** MPD's current playback state */
	enum mpd_state state;

	/** crossfade setting in seconds */
	unsigned crossfade;

	/** Mixramp threshold in dB */
	float mixrampdb;

	/** Mixramp extra delay in seconds */
	float mixrampdelay;

	/**
	 * If a song is currently selected (always the case when state
	 * is PLAY or PAUSE), this is the position of the currently
	 * playing song in the queue, beginning with 0.
	 */
	int song_pos;

	/** Song ID of the currently selected song */
	int song_id;

	/** The same as song_pos, but for the next song to be played */
	int next_song_pos;

	/** Song ID of the next song to be played */
	int next_song_id;

	/**
	 * Time in seconds that have elapsed in the currently
	 * playing/paused song.
	 */
	unsigned elapsed_time;

	/**
	 * Time in milliseconds that have elapsed in the currently
	 * playing/paused song.
	 */
	unsigned elapsed_ms;

	/** length in seconds of the currently playing/paused song */
	unsigned total_time;

	/** current bit rate in kbps */
	unsigned kbit_rate;

	/** the current audio format */
	struct mpd_audio_format audio_format;

	/** non-zero if MPD is updating, 0 otherwise */
	unsigned update_id;

	/** the name of the current partition */
	char *partition;

	/** error message */
	char *error;
};

struct mpd_status *
mpd_status_begin(void)
{
	struct mpd_status *status = malloc(sizeof(*status));
	if (status == NULL)
		return NULL;

	status->volume = -1;
	status->repeat = false;
	status->random = false;
	status->single = MPD_SINGLE_OFF;
	status->consume = false;
	status->queue_version = 0;
	status->queue_length = 0;
	status->state = MPD_STATE_UNKNOWN;
	status->song_pos = -1;
	status->song_id = -1;
	status->next_song_pos = -1;
	status->next_song_id = -1;
	status->elapsed_time = 0;
	status->elapsed_ms = 0;
	status->total_time = 0;
	status->kbit_rate = 0;
	memset(&status->audio_format, 0, sizeof(status->audio_format));
	status->crossfade = 0;
	status->mixrampdb = 100.0;
	status->mixrampdelay = -1.0;
	status->partition = NULL;
	status->error = NULL;
	status->update_id = 0;

	return status;
}

/**
 * Parses the fractional part of the "elapsed" response line.  Up to
 * three digits are parsed.
 */
static unsigned
parse_ms(const char *p)
{
	unsigned ms;

	if (*p >= '0' && *p <= '9')
		ms = 100 * (*p++ - '0');
	else
		return 0;

	if (*p >= '0' && *p <= '9')
		ms += 10 * (*p - '0');
	else
		return ms;

	if (*p >= '0' && *p <= '9')
		ms += *p - '0';

	return ms;
}

static enum mpd_state
parse_mpd_state(const char *p)
{
	if (strcmp(p, "play") == 0)
		return MPD_STATE_PLAY;
	else if (strcmp(p, "stop") == 0)
		return MPD_STATE_STOP;
	else if (strcmp(p, "pause") == 0)
		return MPD_STATE_PAUSE;
	else
		return MPD_STATE_UNKNOWN;
}

enum mpd_single_state
mpd_parse_single_state(const char *p)
{
	if (strcmp(p, "0") == 0)
		return MPD_SINGLE_OFF;
	else if (strcmp(p, "1") == 0)
		return MPD_SINGLE_ON;
	else if (strcmp(p, "oneshot") == 0)
		return MPD_SINGLE_ONESHOT;
	else
		return MPD_SINGLE_UNKNOWN;
}

const char *
mpd_lookup_single_state(enum mpd_single_state state)
{
	switch (state) {
	case MPD_SINGLE_OFF:
		return "0";
	case MPD_SINGLE_ON:
		return "1";
	case MPD_SINGLE_ONESHOT:
		return "oneshot";
	case MPD_SINGLE_UNKNOWN:
		return NULL;
	}
	return NULL;
}

enum mpd_consume_state
mpd_parse_consume_state(const char *p)
{
	if (strcmp(p, "0") == 0)
		return MPD_CONSUME_OFF;
	else if (strcmp(p, "1") == 0)
		return MPD_CONSUME_ON;
	else if (strcmp(p, "oneshot") == 0)
		return MPD_CONSUME_ONESHOT;
	else
		return MPD_CONSUME_UNKNOWN;
}

const char *
mpd_lookup_consume_state(enum mpd_consume_state state)
{
	switch (state) {
	case MPD_CONSUME_OFF:
		return "0";
	case MPD_CONSUME_ON:
		return "1";
	case MPD_CONSUME_ONESHOT:
		return "oneshot";
	default:
		return NULL;
	}
}

void
mpd_status_feed(struct mpd_status *status, const struct mpd_pair *pair)
{
	assert(status != NULL);
	assert(pair != NULL);

	if (strcmp(pair->name, "volume") == 0)
		status->volume = atoi(pair->value);
	else if (strcmp(pair->name, "repeat") == 0)
		status->repeat = !!atoi(pair->value);
	else if (strcmp(pair->name, "random") == 0)
		status->random = !!atoi(pair->value);
	else if (strcmp(pair->name, "single") == 0)
		status->single = mpd_parse_single_state(pair->value);
	else if (strcmp(pair->name, "consume") == 0)
		status->consume = mpd_parse_consume_state(pair->value);
	else if (strcmp(pair->name, "playlist") == 0)
		status->queue_version = strtoul(pair->value, NULL, 10);
	else if (strcmp(pair->name, "playlistlength") == 0)
		status->queue_length = strtoul(pair->value, NULL, 10);
	else if (strcmp(pair->name, "bitrate") == 0)
		status->kbit_rate = strtoul(pair->value, NULL, 10);
	else if (strcmp(pair->name, "state") == 0)
		status->state = parse_mpd_state(pair->value);
	else if (strcmp(pair->name, "song") == 0)
		status->song_pos = (int)strtoimax(pair->value, NULL, 10);
	else if (strcmp(pair->name, "songid") == 0)
		status->song_id = (int)strtoimax(pair->value, NULL, 10);
	else if (strcmp(pair->name, "nextsong") == 0)
		status->next_song_pos = (int)strtoimax(pair->value, NULL, 10);
	else if (strcmp(pair->name, "nextsongid") == 0)
		status->next_song_id = (int)strtoimax(pair->value, NULL, 10);
	else if (strcmp(pair->name, "time") == 0) {
		char *endptr;

		status->elapsed_time = strtoul(pair->value, &endptr, 10);
		if (*endptr == ':')
			status->total_time = strtoul(endptr + 1, NULL, 10);

		if (status->elapsed_ms == 0)
			status->elapsed_ms = status->elapsed_time * 1000;
	} else if (strcmp(pair->name, "elapsed") == 0) {
		char *endptr;

		status->elapsed_ms = strtoul(pair->value, &endptr, 10) * 1000;
		if (*endptr == '.')
			status->elapsed_ms += parse_ms(endptr + 1);

		if (status->elapsed_time == 0)
			status->elapsed_time = status->elapsed_ms / 1000;
	} else if (strcmp(pair->name, "partition") == 0) {
		free(status->partition);
		status->partition = strdup(pair->value);
	} else if (strcmp(pair->name, "error") == 0) {
		free(status->error);
		status->error = strdup(pair->value);
	} else if (strcmp(pair->name, "xfade") == 0)
		status->crossfade = strtoul(pair->value, NULL, 10);
	else if (strcmp(pair->name, "mixrampdb") == 0)
		status->mixrampdb = strtof(pair->value, NULL);
	else if (strcmp(pair->name, "mixrampdelay") == 0)
		status->mixrampdelay = strtof(pair->value, NULL);
	else if (strcmp(pair->name, "updating_db") == 0)
		status->update_id = strtoul(pair->value, NULL, 10);
	else if (strcmp(pair->name, "audio") == 0)
		mpd_parse_audio_format(&status->audio_format, pair->value);
}

void mpd_status_free(struct mpd_status * status)
{
	assert(status != NULL);

	free(status->partition);
	free(status->error);
	free(status);
}

int mpd_status_get_volume(const struct mpd_status *status)
{
	assert(status != NULL);

	return status->volume;
}

bool
mpd_status_get_repeat(const struct mpd_status *status)
{
	assert(status != NULL);

	return status->repeat;
}

bool
mpd_status_get_random(const struct mpd_status *status)
{
	assert(status != NULL);

	return status->random;
}

enum mpd_single_state
mpd_status_get_single_state(const struct mpd_status *status)
{
	assert(status != NULL);

	return status->single;
}

bool
mpd_status_get_single(const struct mpd_status *status)
{
	assert(status != NULL);

	return status->single == MPD_SINGLE_ONESHOT ||
	       status->single == MPD_SINGLE_ON;
}

enum mpd_consume_state
mpd_status_get_consume_state(const struct mpd_status *status)
{
	assert(status != NULL);

	return status->consume;
}

bool
mpd_status_get_consume(const struct mpd_status *status)
{
	assert(status != NULL);

	return status->consume == MPD_CONSUME_ONESHOT ||
	       status->consume == MPD_CONSUME_ON;
}

unsigned
mpd_status_get_queue_length(const struct mpd_status *status)
{
	assert(status != NULL);

	return status->queue_length;
}

unsigned
mpd_status_get_queue_version(const struct mpd_status *status)
{
	assert(status != NULL);

	return status->queue_version;
}

enum mpd_state
mpd_status_get_state(const struct mpd_status *status)
{
	assert(status != NULL);

	return status->state;
}

unsigned
mpd_status_get_crossfade(const struct mpd_status *status)
{
	assert(status != NULL);

	return status->crossfade;
}

float
mpd_status_get_mixrampdb(const struct mpd_status *status)
{
	assert(status != NULL);

	return status->mixrampdb;
}

float
mpd_status_get_mixrampdelay(const struct mpd_status *status)
{
	assert(status != NULL);

	return status->mixrampdelay;
}

int
mpd_status_get_song_pos(const struct mpd_status *status)
{
	assert(status != NULL);

	return status->song_pos;
}

int
mpd_status_get_song_id(const struct mpd_status *status)
{
	assert(status != NULL);

	return status->song_id;
}

int
mpd_status_get_next_song_pos(const struct mpd_status *status)
{
	assert(status != NULL);

	return status->next_song_pos;
}

int
mpd_status_get_next_song_id(const struct mpd_status *status)
{
	return status->next_song_id;
}

unsigned
mpd_status_get_elapsed_time(const struct mpd_status *status)
{
	assert(status != NULL);

	return status->elapsed_time;
}

unsigned
mpd_status_get_elapsed_ms(const struct mpd_status *status)
{
	assert(status != NULL);

	return status->elapsed_ms;
}

unsigned
mpd_status_get_total_time(const struct mpd_status *status)
{
	assert(status != NULL);

	return status->total_time;
}

unsigned
mpd_status_get_kbit_rate(const struct mpd_status *status)
{
	assert(status != NULL);

	return status->kbit_rate;
}

const struct mpd_audio_format *
mpd_status_get_audio_format(const struct mpd_status *status)
{
	assert(status != NULL);

	return !mpd_audio_format_is_empty(&status->audio_format)
		? &status->audio_format
		: NULL;
}

unsigned
mpd_status_get_update_id(const struct mpd_status *status)
{
	assert(status != NULL);

	return status->update_id;
}

const char *
mpd_status_get_partition(const struct mpd_status *status)
{
	assert(status != NULL);

	return status->partition;
}

const char *
mpd_status_get_error(const struct mpd_status *status)
{
	assert(status != NULL);

	return status->error;
}
