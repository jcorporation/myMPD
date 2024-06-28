/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2024 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include "compile_time.h"
#include "src/lib/config_def.h"
#include "utility.h"

#include "dist/utest/utest.h"
#include "dist/libmympdclient/src/isong.h"
#include "src/lib/cache_rax_album.h"
#include "src/mpd_client/tags.h"

#include <mpd/client.h>

#define PCRE2_CODE_UNIT_WIDTH 8
#include <pcre2.h>

UTEST(album_cache, test_album_cache_get_key) {
    struct t_albums_config album_config = {
        .group_tag = MPD_TAG_DATE,
        .mode = ALBUM_MODE_ADV
    };
    struct mpd_song *song = new_song();
    sds key = album_cache_get_key(sdsempty(), song, &album_config);
    ASSERT_STREQ("3efe3b6f830dbcf2a14cd563be79ce37605ef493", key);
    sdsfree(key);

    mympd_mpd_song_add_tag_dedup(song, MPD_TAG_MUSICBRAINZ_ALBUMID, "0c50c04e-994b-4e63-b969-ea82e6b36d3b");
    key = album_cache_get_key(sdsempty(), song, &album_config);
    ASSERT_STREQ("0c50c04e-994b-4e63-b969-ea82e6b36d3b", key);
    sdsfree(key);

    mpd_song_free(song);
}

UTEST(album_cache, test_album_cache_copy_tags) {
    struct mpd_song *song = new_song();
    bool rc = album_cache_copy_tags(song, MPD_TAG_ARTIST, MPD_TAG_ALBUM_ARTIST);
    ASSERT_TRUE(rc);
    const char *value = mpd_song_get_tag(song, MPD_TAG_ALBUM_ARTIST, 0);
    ASSERT_STREQ("Einstürzende Neubauten", value);
    value = mpd_song_get_tag(song, MPD_TAG_ALBUM_ARTIST, 1);
    ASSERT_STREQ("Blixa Bargeld", value);
    mpd_song_free(song);
}

UTEST(album_cache, test_album_cache_set_discs) {
    struct mpd_song *album = new_song();
    struct mpd_song *song = new_song();

    free(song->tags[MPD_TAG_DISC].value);
    song->tags[MPD_TAG_DISC].value = strdup("04");

    album_cache_set_discs(album, song);
    ASSERT_EQ((unsigned) 4, mpd_song_get_pos(album));
    ASSERT_EQ((unsigned) 4, album_get_discs(album));

    free(song->tags[MPD_TAG_DISC].value);
    song->tags[MPD_TAG_DISC].value = strdup("02");

    album_cache_set_discs(album, song);
    ASSERT_EQ((unsigned) 4, mpd_song_get_pos(album));
    ASSERT_EQ((unsigned) 4, album_get_discs(album));

    mpd_song_free(album);
    mpd_song_free(song);
}

UTEST(album_cache, test_album_cache_set_last_modified) {
    struct mpd_song *album = new_song();
    struct mpd_song *song = new_song();
    
    song->last_modified = 1699304602;
    album_cache_set_last_modified(album, song);
    ASSERT_EQ(1699304602, mpd_song_get_last_modified(album));

    song->last_modified = 1000;
    album_cache_set_last_modified(album, song);
    ASSERT_EQ(1699304602, mpd_song_get_last_modified(album));

    mpd_song_free(album);
    mpd_song_free(song);
}

UTEST(album_cache, test_album_cache_set_added) {
    struct mpd_song *album = new_song();
    struct mpd_song *song = new_song();
    
    song->added = 1699304602;
    album_cache_set_added(album, song);
    ASSERT_EQ(1699304451, mpd_song_get_added(album));

    song->added = 1000;
    album_cache_set_added(album, song);
    ASSERT_EQ(1000, mpd_song_get_added(album));

    mpd_song_free(album);
    mpd_song_free(song);
}

UTEST(album_cache, test_album_cache_inc_total_time) {
    struct mpd_song *album = new_song();
    struct mpd_song *song = new_song();
    
    song->duration = 20;
    album_cache_inc_total_time(album, song);
    ASSERT_EQ((unsigned)30, album_get_total_time(album));

    mpd_song_free(album);
    mpd_song_free(song);
}

UTEST(album_cache, test_album_cache_inc_song_count) {
    struct mpd_song *album = new_song();
    album_cache_set_song_count(album, 1);

    unsigned song_count = album_get_song_count(album);
    ASSERT_EQ((unsigned)1, song_count);
    //song count maps to prio
    unsigned prio = mpd_song_get_prio(album);
    ASSERT_EQ((unsigned)1, prio);

    album_cache_inc_song_count(album);
    song_count = album_get_song_count(album);
    ASSERT_EQ((unsigned)2, song_count);

    mpd_song_free(album);
}
