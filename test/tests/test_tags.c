/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2025 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include "compile_time.h"
#include "src/lib/config_def.h"
#include "utility.h"

#include "dist/utest/utest.h"
#include "src/lib/mpdclient.h"
#include "src/mympd_client/tags.h"



#define PCRE2_CODE_UNIT_WIDTH 8
#include <pcre2.h>

UTEST(tags, test_mympd_mpd_song_add_tag_dedup) {
    struct mpd_song *song = new_test_song();
    ASSERT_STREQ("Einstürzende Neubauten", mpd_song_get_tag(song, MPD_TAG_ARTIST, 0));
    ASSERT_STREQ("Blixa Bargeld", mpd_song_get_tag(song, MPD_TAG_ARTIST, 1));
    bool b = mpd_song_get_tag(song, MPD_TAG_ARTIST, 2) == NULL ? true : false;
    ASSERT_TRUE(b);
    ASSERT_STREQ("Einstürzende Neubauten", mpd_song_get_tag(song, MPD_TAG_ALBUM_ARTIST, 0));
    ASSERT_STREQ("Tabula Rasa", mpd_song_get_tag(song, MPD_TAG_ALBUM, 0));
    ASSERT_STREQ("Tabula Rasa", mpd_song_get_tag(song, MPD_TAG_TITLE, 0));
    ASSERT_STREQ("01", mpd_song_get_tag(song, MPD_TAG_TRACK, 0));
    mpd_song_free(song);
}

UTEST(tags, test_is_multivalue_tag) {
    ASSERT_TRUE(is_multivalue_tag(MPD_TAG_ARTIST));
    ASSERT_FALSE(is_multivalue_tag(MPD_TAG_TRACK));
}

UTEST(tags, test_get_sort_tag) {
    struct t_mympd_mpd_tags tags;
    mympd_mpd_tags_reset(&tags);
    tags.len = 4;
    tags.tags[0] = MPD_TAG_ALBUM;
    tags.tags[1] = MPD_TAG_ALBUM_SORT;
    tags.tags[2] = MPD_TAG_PERFORMER;
    tags.tags[3] = MPD_TAG_TITLE;
    ASSERT_EQ(MPD_TAG_ALBUM_SORT, get_sort_tag(MPD_TAG_ALBUM, &tags));
    ASSERT_EQ(MPD_TAG_PERFORMER, get_sort_tag(MPD_TAG_PERFORMER, &tags));
}

UTEST(tags, test_mympd_client_get_tag_value_string) {
    struct mpd_song *song = new_test_song();
    sds s = mympd_client_get_tag_value_string(song, MPD_TAG_ARTIST, sdsempty());
    ASSERT_STREQ("Einstürzende Neubauten, Blixa Bargeld", s);
    sdsclear(s);
    s = mympd_client_get_tag_value_string(song, MPD_TAG_PERFORMER, s);
    ASSERT_STREQ("", s);
    sdsfree(s);
    mpd_song_free(song);
}

UTEST(tags, test_mympd_client_get_tag_values) {
    struct mpd_song *song = new_test_song();
    sds s = mympd_client_get_tag_values(song, MPD_TAG_ARTIST, sdsempty());
    ASSERT_STREQ("[\"Einstürzende Neubauten\",\"Blixa Bargeld\"]", s);
    sdsclear(s);
    s = mympd_client_get_tag_values(song, MPD_TAG_TITLE, s);
    ASSERT_STREQ("\"Tabula Rasa\"", s);
    sdsclear(s);
    s = mympd_client_get_tag_values(song, MPD_TAG_PERFORMER, s);
    ASSERT_STREQ("[]", s);
    sdsclear(s);
    s = mympd_client_get_tag_values(song, MPD_TAG_DATE, s);
    ASSERT_STREQ("\"\"", s);
    sdsfree(s);
    mpd_song_free(song);
}

UTEST(tags, test_check_tags) {
    sds s = sdsnew("Artist, Album,Title");
    struct t_mympd_mpd_tags tags;
    mympd_mpd_tags_reset(&tags);
    struct t_mympd_mpd_tags allowed;
    mympd_mpd_tags_reset(&allowed);
    allowed.len++;
    allowed.tags[0] = MPD_TAG_ALBUM;
    allowed.len++;
    allowed.tags[1] = MPD_TAG_TITLE;
    check_tags(s, "taglist", &tags, &allowed);
    ASSERT_EQ(2, (int)tags.len);
    ASSERT_EQ(MPD_TAG_ALBUM, tags.tags[0]);
    ASSERT_EQ(MPD_TAG_TITLE, tags.tags[1]);
    sdsfree(s);
}

UTEST(tags, test_mympd_client_tag_exists) {
    struct t_mympd_mpd_tags tags;
    mympd_mpd_tags_reset(&tags);
    tags.len++;
    tags.tags[0] = MPD_TAG_ALBUM;
    tags.len++;
    tags.tags[1] = MPD_TAG_ARTIST;
    ASSERT_TRUE(mympd_client_tag_exists(&tags, MPD_TAG_ALBUM));
    ASSERT_FALSE(mympd_client_tag_exists(&tags, MPD_TAG_ALBUM_ARTIST));
}
