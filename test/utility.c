/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2024 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include "utility.h"

#include "dist/libmympdclient/src/isong.h"
#include "src/lib/filehandler.h"
#include "src/mpd_client/tags.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

void init_testenv(void) {
    mkdir("/tmp/mympd-test", 0770);
    mkdir("/tmp/mympd-test/state", 0770);
    mkdir("/tmp/mympd-test/state/default", 0770);
    mkdir("/tmp/mympd-test/webradios", 0770);
    unsetenv("TESTVAR");
}

void clean_testenv(void) {
    if (system("rm -rf /tmp/mympd-test") != 0) {
        printf("Failure cleaning /tmp/mympd-test\n");
    }
    unsetenv("TESTVAR");
}

bool create_testfile(void) {
    sds file = sdsnew("/tmp/mympd-test/state/test");
    const char *data ="asdfjlkasdfjklsafd\nasfdsdfawaerwer\n";
    size_t len = strlen(data);
    bool rc = write_data_to_file(file, data, len);
    sdsfree(file);
    return rc;
}

struct mpd_song *new_song(void) {
    struct mpd_song *song = malloc(sizeof(struct mpd_song));
    song->uri = strdup("/music/test.mp3");

    for (unsigned i = 0; i < MPD_TAG_COUNT; ++i) {
        song->tags[i].value = NULL;
    }
    mympd_mpd_song_add_tag_dedup(song, MPD_TAG_ARTIST, "Einstürzende Neubauten");
    mympd_mpd_song_add_tag_dedup(song, MPD_TAG_ARTIST, "Blixa Bargeld");
    mympd_mpd_song_add_tag_dedup(song, MPD_TAG_ARTIST, "Blixa Bargeld");
    mympd_mpd_song_add_tag_dedup(song, MPD_TAG_ALBUM_ARTIST, "Einstürzende Neubauten");
    mympd_mpd_song_add_tag_dedup(song, MPD_TAG_ALBUM, "Tabula Rasa");
    mympd_mpd_song_add_tag_dedup(song, MPD_TAG_TITLE, "Tabula Rasa");
    mympd_mpd_song_add_tag_dedup(song, MPD_TAG_TRACK, "01");
    mympd_mpd_song_add_tag_dedup(song, MPD_TAG_DISC, "01");

    song->duration = 10;
    song->duration_ms = 10000;
    song->start = 0;
    song->end = 0;
    song->last_modified = 1699304451;
    song->added = 1699304451;
    song->pos = 0;
    song->id = 0;
    song->prio = 0;

    memset(&song->audio_format, 0, sizeof(song->audio_format));
    song->audio_format.channels = 2;
    song->audio_format.sample_rate = 44100;
    song->audio_format.bits = 24;

#ifndef NDEBUG
    song->finished = false;
#endif

    return song;
}
