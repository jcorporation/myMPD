/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2026 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#ifndef TEST_MPD_SONG_H
#define TEST_MPD_SONG_H

#include "src/lib/mpdclient.h"

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
     * The POSIX UTC time stamp of database addition, or 0 if
     * that is unknown.
     */
    time_t added;

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

#endif
