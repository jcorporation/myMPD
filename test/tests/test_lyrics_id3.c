/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2025 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include "compile_time.h"
#include "utility.h"

#include "dist/utest/utest.h"
#include "src/lib/config/config.h"
#include "src/lib/config/mympd_state.h"
#include "src/lib/json/json_query.h"
#include "src/webserver/lyrics_id3.h"

UTEST(lyrics_id3, test_lyricsextract_unsynced_id3) {
    struct t_list extracted;
    list_init(&extracted);
    //testfile has unsynced lyrics
    sds mediafile = sdsnew(MYMPD_BUILD_DIR"/testfiles/test.mp3");
    lyricsextract_unsynced_id3(&extracted, mediafile);
    ASSERT_EQ(extracted.length, 1U);
    list_clear(&extracted);
    sdsfree(mediafile);
}
