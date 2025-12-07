/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2025 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include "compile_time.h"
#include "utility.h"

#include "dist/utest/utest.h"
#include "src/lib/config.h"
#include "src/lib/json/json_query.h"
#include "src/lib/mympd_state.h"
#include "src/webserver/lyrics_flac.h"

UTEST(lyrics_flac, test_mympd_api_lyrics_get) {
        struct t_list extracted;
    list_init(&extracted);
    //testfile has unsynced lyrics
    sds mediafile = sdsnew(MYMPD_BUILD_DIR"/testfiles/test.flac");
    lyricsextract_flac(&extracted, mediafile, false, MYMPD_LYRICS_VORBIS_USLT, false);
    ASSERT_EQ(extracted.length, 1U);
    list_clear(&extracted);
    sdsfree(mediafile);
}
