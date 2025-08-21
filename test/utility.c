/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2025 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include "utility.h"

#include "src/lib/album.h"
#include "src/lib/filehandler.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

void init_testenv(void) {
    mkdir("/tmp/mympd-test", 0770);
    mkdir("/tmp/mympd-test/http", 0770);
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
    const char *data = TESTFILE_CONTENT"\n";
    size_t len = strlen(data);
    bool rc = write_data_to_file(file, data, len);
    sdsfree(file);
    return rc;
}

bool song_append_tag(struct mpd_song *song, enum mpd_tag_type type, const char *value) {
    struct mpd_tag_value *tag = &song->tags[type];

    if ((int)type < 0 ||
        type >= MPD_TAG_COUNT)
    {
        return false;
    }

    if (tag->value == NULL) {
        tag->next = NULL;
        tag->value = strdup(value);
        if (tag->value == NULL) {
            return false;
        }
    }
    else {
        while (tag->next != NULL) {
            if (strcmp(tag->value, value) == 0) {
                //do not add duplicate values
                return true;
            }
            tag = tag->next;
        }
        if (strcmp(tag->value, value) == 0) {
            //do not add duplicate values
            return true;
        }
        struct mpd_tag_value *prev = tag;
        tag = malloc(sizeof(*tag));
        if (tag == NULL) {
            return false;
        }

        tag->value = strdup(value);
        if (tag->value == NULL) {
            free(tag);
            return false;
        }

        tag->next = NULL;
        prev->next = tag;
    }

    return true;
}

struct t_album *new_test_album(void) {
    struct mpd_song *song = new_test_song();
    const struct t_mympd_mpd_tags album_tags = {
        .len = 3,
        .tags = {MPD_TAG_ARTIST, MPD_TAG_ALBUM_ARTIST, MPD_TAG_ALBUM }
    };
    struct t_album *album = album_new_from_song(song, &album_tags);
    mpd_song_free(song);
    return album;
}

struct mpd_song *new_test_song(void) {
    struct mpd_song *song = malloc(sizeof(struct mpd_song));
    song->uri = strdup("music/test.mp3");

    for (unsigned i = 0; i < MPD_TAG_COUNT; ++i) {
        song->tags[i].value = NULL;
    }
    song_append_tag(song, MPD_TAG_ARTIST, "Einstürzende Neubauten");
    song_append_tag(song, MPD_TAG_ARTIST, "Blixa Bargeld");
    song_append_tag(song, MPD_TAG_ARTIST, "Blixa Bargeld");
    song_append_tag(song, MPD_TAG_ALBUM_ARTIST, "Einstürzende Neubauten");
    song_append_tag(song, MPD_TAG_ALBUM, "Tabula Rasa");
    song_append_tag(song, MPD_TAG_TITLE, "Tabula Rasa");
    song_append_tag(song, MPD_TAG_TRACK, "01");
    song_append_tag(song, MPD_TAG_DISC, "01");

    song->duration = 10;
    song->duration_ms = 10000;
    song->start = 0;
    song->end = 0;
    song->last_modified = 1699304451;
    song->added = 1699304451;
    song->pos = 0;
    song->id = 0;
    song->prio = 10;

    song->audio_format.sample_rate = 44100;
    song->audio_format.bits = 24;
    song->audio_format.channels = 2;

#ifndef NDEBUG
    song->finished = false;
#endif

    return song;
}
