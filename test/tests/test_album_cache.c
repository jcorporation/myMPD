/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2025 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include "compile_time.h"
#include "src/lib/album.h"
#include "src/lib/config_def.h"
#include "utility.h"

#include "dist/utest/utest.h"
#include "src/lib/cache/cache_rax_album.h"

#include <mpd/client.h>

#define PCRE2_CODE_UNIT_WIDTH 8
#include <pcre2.h>

UTEST(album_cache, test_album_cache_get_key) {
    struct t_albums_config album_config = {
        .group_tag = MPD_TAG_DATE,
        .mode = ALBUM_MODE_ADV
    };
    struct mpd_song *song = new_test_song();
    sds key = album_cache_get_key(sdsempty(), song, &album_config);
    ASSERT_STREQ("3efe3b6f830dbcf2a14cd563be79ce37605ef493", key);
    sdsfree(key);

    song_append_tag(song, MPD_TAG_MUSICBRAINZ_ALBUMID, "0c50c04e-994b-4e63-b969-ea82e6b36d3b");
    key = album_cache_get_key(sdsempty(), song, &album_config);
    ASSERT_STREQ("0c50c04e-994b-4e63-b969-ea82e6b36d3b", key);
    sdsfree(key);

    album_free(song);
}

UTEST(album_cache, test_album_cache_copy_tags) {
    struct mpd_song *album = new_test_album();
    bool rc = album_copy_tags(album, MPD_TAG_ARTIST, MPD_TAG_ALBUM_ARTIST);
    ASSERT_TRUE(rc);
    const char *value = album_get_tag(album, MPD_TAG_ALBUM_ARTIST, 0);
    ASSERT_STREQ("Einstürzende Neubauten", value);
    value = mpd_song_get_tag(album, MPD_TAG_ALBUM_ARTIST, 1);
    ASSERT_STREQ("Blixa Bargeld", value);
    album_free(album);
}

UTEST(album_cache, test_album_cache_set_discs) {
    struct mpd_song *album = new_test_album();

    album_set_discs(album, "04");
    ASSERT_EQ((unsigned) 4, album_get_discs(album));

    album_set_discs(album, "02");
    ASSERT_EQ((unsigned) 4, album_get_discs(album));

    album_free(album);
}

UTEST(album_cache, test_album_cache_set_last_modified) {
    struct mpd_song *album = new_test_album();

    album_set_last_modified(album, 1699304602);
    ASSERT_EQ(1699304602, mpd_song_get_last_modified(album));

    album_set_last_modified(album, 1000);
    ASSERT_EQ(1699304602, mpd_song_get_last_modified(album));

    album_free(album);
}

UTEST(album_cache, test_album_cache_set_added) {
    struct mpd_song *album = new_test_album();

    album_set_added(album, 1699304602);
    ASSERT_EQ(1699304451, album_get_added(album));

    album_set_added(album, 1000);
    ASSERT_EQ(1000, mpd_song_get_added(album));

    album_free(album);
}

UTEST(album_cache, test_album_cache_inc_total_time) {
    struct mpd_song *album = new_test_album();

    album_inc_total_time(album, 20);
    ASSERT_EQ((unsigned)30, album_get_total_time(album));

    album_free(album);
}

UTEST(album_cache, test_album_cache_inc_song_count) {
    struct mpd_song *album = new_test_album();
    album_set_song_count(album, 1);

    unsigned song_count = album_get_song_count(album);
    ASSERT_EQ((unsigned)1, song_count);

    album_inc_song_count(album);
    song_count = album_get_song_count(album);
    ASSERT_EQ((unsigned)2, song_count);

    mpd_song_free(album);
}

UTEST(album_cache, test_album_cache_set_uri) {
    struct mpd_song *album = new_test_album();
    album_set_uri(album, "/newuri");
    ASSERT_STREQ("/newuri", album_get_uri(album));
    album_free(album);
}
