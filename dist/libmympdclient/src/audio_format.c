// SPDX-License-Identifier: BSD-3-Clause
// Copyright The Music Player Daemon Project

#include "iaf.h"
#include <mpd/audio_format.h>

#include <stdlib.h>
#include <string.h>

void
mpd_parse_audio_format(struct mpd_audio_format *audio_format, const char *p)
{
	char *endptr;

	if (strncmp(p, "dsd", 3) == 0) {
		/* allow format specifications such as "dsd64" which
		   implies the sample rate */

		unsigned long dsd = strtoul(p + 3, &endptr, 10);
		if (endptr > p + 3 && *endptr == ':' &&
		    dsd >= 32 && dsd <= 4096 && dsd % 2 == 0) {
			audio_format->sample_rate = dsd * 44100 / 8;
			audio_format->bits = MPD_SAMPLE_FORMAT_DSD;

			p = endptr + 1;
			audio_format->channels = (uint8_t)strtoul(p, NULL, 10);
			return;
		}
	}

	audio_format->sample_rate = strtoul(p, &endptr, 10);
	if (*endptr == ':') {
		p = endptr + 1;

		if (p[0] == 'f' && p[1] == ':') {
			audio_format->bits = MPD_SAMPLE_FORMAT_FLOAT;
			p += 2;
		} else if (p[0] == 'd' && p[1] == 's' &&
			   p[2] == 'd' && p[3] == ':') {
			audio_format->bits = MPD_SAMPLE_FORMAT_DSD;
			p += 4;
		} else {
			audio_format->bits = (uint8_t)strtoul(p, &endptr, 10);
			p = *endptr == ':' ? endptr + 1 : NULL;
		}

		audio_format->channels = p != NULL
			? (uint8_t)strtoul(p, NULL, 10)
			: 0;
	} else {
		audio_format->bits = 0;
		audio_format->channels = 0;
	}
}
