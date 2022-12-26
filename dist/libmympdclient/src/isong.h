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

   - Neither the name of the Music Player Daemon nor the names of its
   contributors may be used to endorse or promote products derived from
   this software without specific prior written permission.

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

#ifndef MPD_ISONG_H
#define MPD_ISONG_H

#include <mpd/tag.h>
#include <mpd/audio_format.h>

#include <stdbool.h>

struct mpd_tag_value {
	struct mpd_tag_value *next;

	char *value;
};

struct mpd_song {
	char *uri;

	struct mpd_tag_value tags[MPD_TAG_COUNT];

	/**
	 * Duration of the song in seconds, or 0 for unknown.
	 */
	unsigned duration;

	/**
	 * Duration of the song in milliseconds, or 0 for unknown.
	 */
	unsigned duration_ms;

	/**
	 * Start of the virtual song within the physical file in
	 * seconds.
	 */
	unsigned start;

	/**
	 * End of the virtual song within the physical file in
	 * seconds.  Zero means that the physical song file is
	 * played to the end.
	 */
	unsigned end;

	/**
	 * The POSIX UTC time stamp of the last modification, or 0 if
	 * that is unknown.
	 */
	time_t last_modified;

	/**
	 * The position of this song within the queue.
	 */
	unsigned pos;

	/**
	 * The id of this song within the queue.
	 */
	unsigned id;

	/**
	 * The priority of this song within the queue.
	 */
	unsigned prio;

#ifndef NDEBUG
	/**
	 * This flag is used in an assertion: when it is set, you must
	 * not call mpd_song_feed() again.  It is a safeguard for
	 * buggy callers.
	 */
	bool finished;
#endif

	/**
	 * The audio format as reported by MPD's decoder plugin.
	 */
	struct mpd_audio_format audio_format;
};

struct mpd_song *
mpd_song_new(const char *uri);

#endif
