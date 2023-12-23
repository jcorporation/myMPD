// SPDX-License-Identifier: BSD-2-Clause
// Copyright The Music Player Daemon Project

#ifndef MPD_INTERNAL_AUDIO_FORMAT_H
#define MPD_INTERNAL_AUDIO_FORMAT_H

#include <mpd/audio_format.h>

#include <stdbool.h>

struct mpd_audio_format;

void
mpd_parse_audio_format(struct mpd_audio_format *audio_format, const char *p);

static inline bool
mpd_audio_format_is_empty(const struct mpd_audio_format *audio_format)
{
	return audio_format->sample_rate == 0 &&
		audio_format->bits == MPD_SAMPLE_FORMAT_UNDEFINED &&
		audio_format->channels == 0;
}

#endif
