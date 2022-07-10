/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2022 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include "mympd_config_defs.h"

#include "../../dist/utest/utest.h"
#include <mpd/client.h>
#include "../../dist/libmpdclient/src/isong.h"
#include "../../src/mpd_client/mpd_client_search_local.h"
#include "../../src/mpd_client/mpd_client_tags.h"

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

	song->duration = 10;
	song->duration_ms = 10000;
	song->start = 0;
	song->end = 0;
	song->last_modified = 2000;
	song->pos = 1;
	song->id = 1;
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

UTEST(mpd_client_tags, test_mympd_mpd_song_set_last_modified) {
    struct mpd_song *song = new_song();
    ASSERT_EQ(2000, mpd_song_get_last_modified(song));
    mympd_mpd_song_set_last_modified(song, 3000);
    ASSERT_EQ(3000, mpd_song_get_last_modified(song));
    mpd_song_free(song);
}

UTEST(mpd_client_tags, test_mympd_mpd_song_add_tag_dedup) {
    struct mpd_song *song = new_song();
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

UTEST(mpd_client_tags, test_is_multivalue_tag) {
    ASSERT_TRUE(is_multivalue_tag(MPD_TAG_ARTIST));
    ASSERT_FALSE(is_multivalue_tag(MPD_TAG_TRACK));
}

UTEST(mpd_client_tags, test_get_sort_tag) {
    ASSERT_EQ(MPD_TAG_ALBUM_SORT, get_sort_tag(MPD_TAG_ALBUM));
    ASSERT_EQ(MPD_TAG_PERFORMER, get_sort_tag(MPD_TAG_PERFORMER));
}

UTEST(mpd_client_tags, test_mpd_client_get_tag_value_string) {
    struct mpd_song *song = new_song();
    sds s = mpd_client_get_tag_value_string(song, MPD_TAG_ARTIST, sdsempty());
    ASSERT_STREQ("Einstürzende Neubauten, Blixa Bargeld", s);
    sdsclear(s);
    s = mpd_client_get_tag_value_string(song, MPD_TAG_DISC, s);
    ASSERT_STREQ("", s);
    sdsfree(s);
    mpd_song_free(song);
}

UTEST(mpd_client_tags, test_mpd_client_get_tag_values) {
    struct mpd_song *song = new_song();
    sds s = mpd_client_get_tag_values(song, MPD_TAG_ARTIST, sdsempty());
    ASSERT_STREQ("[\"Einstürzende Neubauten\",\"Blixa Bargeld\"]", s);
    sdsclear(s);
    s = mpd_client_get_tag_values(song, MPD_TAG_TITLE, s);
    ASSERT_STREQ("\"Tabula Rasa\"", s);
    sdsclear(s);
    s = mpd_client_get_tag_values(song, MPD_TAG_PERFORMER, s);
    ASSERT_STREQ("[\"-\"]", s);
    sdsclear(s);
    s = mpd_client_get_tag_values(song, MPD_TAG_DISC, s);
    ASSERT_STREQ("\"-\"", s);
    sdsfree(s);
    mpd_song_free(song);
}

UTEST(mpd_client_tags, test_check_tags) {
    sds s = sdsnew("Artist, Album,Title");
    struct t_tags tags;
    reset_t_tags(&tags);
    struct t_tags allowed;
    reset_t_tags(&allowed);
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

UTEST(mpd_client_tags, test_mpd_client_tag_exists) {
    struct t_tags tags;
    reset_t_tags(&tags);
    tags.len++;
    tags.tags[0] = MPD_TAG_ALBUM;
    tags.len++;
    tags.tags[1] = MPD_TAG_ARTIST;
    ASSERT_TRUE(mpd_client_tag_exists(&tags, MPD_TAG_ALBUM));
    ASSERT_FALSE(mpd_client_tag_exists(&tags, MPD_TAG_ALBUM_ARTIST));
}

UTEST(mpd_client_search_local, test_search_mpd_song) {
    struct mpd_song *song = new_song();
    struct t_tags tags;
    reset_t_tags(&tags);
    tags.len++;
    tags.tags[0] = MPD_TAG_ALBUM;
    sds s = sdsnew("tabula");
    ASSERT_TRUE(search_mpd_song(song, s, &tags));
    sdsclear(s);
    s = sdscat(s, "neu");
    ASSERT_FALSE(search_mpd_song(song, s, &tags));
    sdsfree(s);
    mpd_song_free(song);
}

bool search_by_expression(const char *expr_string) {
    struct mpd_song *song = new_song();
    //browse tag types
    struct t_tags tags;
    reset_t_tags(&tags);
    tags.len++;
    tags.tags[0] = MPD_TAG_ALBUM;
    tags.len++;
    tags.tags[1] = MPD_TAG_ARTIST;

    sds expression = sdsnew(expr_string);
    struct t_list *expr_list = parse_search_expression_to_list(expression);
    sdsfree(expression);
    bool rc = search_song_expression(song, expr_list, &tags);
    free_search_expression_list(expr_list);
    free(expr_list);
    mpd_song_free(song);
    return rc;
}

UTEST(mpd_client_search_local, test_search_mpd_song_expression) {
    //tag with single value
    ASSERT_TRUE(search_by_expression("((Album contains 'tabula'))"));    //containing string
    ASSERT_TRUE(search_by_expression("((Album starts_with 'TABULA'))")); //starting string
    ASSERT_TRUE(search_by_expression("((Album == 'Tabula Rasa'))"));     //exact match
    ASSERT_TRUE(search_by_expression("((Album =~ 'Tab.*'))"));           //regex match

    ASSERT_FALSE(search_by_expression("((Album != 'Tabula Rasa'))"));    //not exact match
    ASSERT_FALSE(search_by_expression("((Album !~ 'Tabula.*'))"));       //regex mismatch

    //tag with multiple values
    ASSERT_TRUE(search_by_expression("((Artist contains 'XA'))"));       //containing string
    ASSERT_TRUE(search_by_expression("((Artist starts_with 'bl'))"));    //starting string
    ASSERT_TRUE(search_by_expression("((Artist == 'Blixa Bargeld'))"));  //exact match
    ASSERT_TRUE(search_by_expression("((Artist =~ 'Blixa.*'))"));        //regex match

    ASSERT_FALSE(search_by_expression("((Artist != 'Blixa Bargeld'))")); //not exact match
    ASSERT_FALSE(search_by_expression("((Artist !~ 'Blixa.*'))")); //regex mismatch
}
