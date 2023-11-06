/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2023 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include "compile_time.h"
#include "src/lib/config_def.h"
#include "utility.h"

#include "dist/utest/utest.h"
#include "dist/libmympdclient/src/isong.h"
#include "src/mpd_client/search_local.h"
#include "src/mpd_client/tags.h"

#include <mpd/client.h>

#define PCRE2_CODE_UNIT_WIDTH 8
#include <pcre2.h>

UTEST(search_local, test_search_mpd_song) {
    struct mpd_song *song = new_song();
    struct t_tags tags;
    reset_t_tags(&tags);
    tags.tags_len++;
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
    mympd_mpd_song_add_tag_dedup(song, MPD_TAG_ARTIST, "MG's");
    //browse tag types
    struct t_tags tags;
    reset_t_tags(&tags);
    tags.tags_len++;
    tags.tags[0] = MPD_TAG_ALBUM;
    tags.tags_len++;
    tags.tags[1] = MPD_TAG_ARTIST;

    sds expression = sdsnew(expr_string);
    struct t_list *expr_list = parse_search_expression_to_list(expression);
    sdsfree(expression);
    bool rc = search_song_expression(song, expr_list, &tags);
    free_search_expression_list(expr_list);
    mpd_song_free(song);
    return rc;
}

UTEST(search_local, test_search_mpd_song_expression) {
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
    ASSERT_FALSE(search_by_expression("((Artist !~ 'Blixa.*'))"));       //regex mismatch

    ASSERT_TRUE(search_by_expression("((Artist contains 'MG\\'s'))"));   //escaping
    ASSERT_FALSE(search_by_expression("((Artist contains 'MGs\\))"));   //escaping
}
