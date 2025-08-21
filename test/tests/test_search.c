/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2025 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include "compile_time.h"
#include "src/lib/config_def.h"
#include "utility.h"

#include "dist/utest/utest.h"
#include "src/lib/search.h"

#include <mpd/client.h>

#define PCRE2_CODE_UNIT_WIDTH 8
#include <pcre2.h>

bool search_by_expression(const char *expr_string) {
    struct mpd_song *song = new_test_song();
    song_append_tag(song, MPD_TAG_ARTIST, "MG's");
    //browse tag types
    struct t_mympd_mpd_tags tags;
    mympd_mpd_tags_reset(&tags);
    tags.len++;
    tags.tags[0] = MPD_TAG_ALBUM;
    tags.len++;
    tags.tags[1] = MPD_TAG_ARTIST;

    sds expression = sdsnew(expr_string);
    struct t_list *expr_list = parse_search_expression_to_list(expression, SEARCH_TYPE_SONG);
    sdsfree(expression);
    if (expr_list == NULL) {
        mpd_song_free(song);
        return false;
    }
    bool rc = search_expression_song(song, expr_list, &tags);
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

    //double quote
    ASSERT_TRUE(search_by_expression("((Artist contains \"XA\"))"));       //containing string

    //tag with multiple values
    ASSERT_TRUE(search_by_expression("((Artist contains 'XA'))"));       //containing string
    ASSERT_TRUE(search_by_expression("((Artist starts_with 'bl'))"));    //starting string
    ASSERT_TRUE(search_by_expression("((Artist == 'Blixa Bargeld'))"));  //exact match
    ASSERT_TRUE(search_by_expression("((Artist =~ 'Blixa.*'))"));        //regex match
    ASSERT_FALSE(search_by_expression("((Artist != 'Blixa Bargeld'))")); //not exact match
    ASSERT_FALSE(search_by_expression("((Artist !~ 'Blixa.*'))"));       //regex mismatch

    //special any tag
    ASSERT_TRUE(search_by_expression("((any contains 'XA'))"));       //containing string
    ASSERT_TRUE(search_by_expression("((any starts_with 'bl'))"));    //starting string
    ASSERT_TRUE(search_by_expression("((any == 'Blixa Bargeld'))"));  //exact match
    ASSERT_TRUE(search_by_expression("((any =~ 'Blixa.*'))"));        //regex match

    //prio
    ASSERT_TRUE(search_by_expression("((prio >= 5))"));
    ASSERT_TRUE(search_by_expression("((prio >= 10))"));
    ASSERT_FALSE(search_by_expression("((prio >= 20))"));

    //audioformat
    ASSERT_TRUE(search_by_expression("((AudioFormat == '44100:24:2'))"));
    ASSERT_TRUE(search_by_expression("((AudioFormat =~ '*:24:2'))"));

    //escaping
    ASSERT_TRUE(search_by_expression("((Artist contains 'MG\\'s'))"));
    ASSERT_FALSE(search_by_expression("((Artist contains 'MGs\\'))"));

    //without operators
    ASSERT_TRUE(search_by_expression("((modified-since '2023-10-10'))"));
    ASSERT_FALSE(search_by_expression("((modified-since '2023-11-17'))"));

    ASSERT_TRUE(search_by_expression("((added-since '2023-10-10'))"));
    ASSERT_FALSE(search_by_expression("((added-since '2023-11-17'))"));

    ASSERT_TRUE(search_by_expression("((base 'music/'))"));
    ASSERT_FALSE(search_by_expression("((base 'abc/'))"));

    ASSERT_TRUE(search_by_expression("((file 'music/test.mp3'))"));
    ASSERT_FALSE(search_by_expression("((file 'abc/abc.mp3'))"));
}

long try_parse(const char *expr) {
    sds expression = sdsnew(expr);
    struct t_list *expr_list = parse_search_expression_to_list(expression, SEARCH_TYPE_SONG);
    sdsfree(expression);
    if (expr_list == NULL) {
        return 0;
    }
    long len = expr_list->length;
    free_search_expression_list(expr_list);
    return len;
}

UTEST(search_local, test_parse_expression) {
    ASSERT_EQ(0, try_parse("asdf"));
    ASSERT_EQ(0, try_parse("(asdf)"));
    ASSERT_EQ(0, try_parse("((asdf))"));
    ASSERT_EQ(0, try_parse("((asdf ="));
    ASSERT_EQ(0, try_parse("((asdf =="));
    ASSERT_EQ(0, try_parse("((asdf == "));
    ASSERT_EQ(0, try_parse("((adsf == '"));
    ASSERT_EQ(0, try_parse("((asdf == 'asdf"));
    ASSERT_EQ(0, try_parse("((Artist == '"));
    ASSERT_EQ(0, try_parse("((Artist == 'asdf"));
    ASSERT_EQ(0, try_parse("((modified-since == 'asdf"));
    ASSERT_EQ(0, try_parse("((modified-since 'asdf"));
    ASSERT_EQ(0, try_parse("((added-since == 'asdf"));
    ASSERT_EQ(0, try_parse("((added-since 'asdf"));
}
