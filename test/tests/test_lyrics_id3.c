/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2024 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include "compile_time.h"
#include "utility.h"

#include "dist/utest/utest.h"
#include "src/lib/jsonrpc.h"
#include "src/lib/mympd_state.h"
#include "src/mympd_api/lyrics.h"

UTEST(lyrics_id3, test_mympd_api_lyrics_get) {
    sds music_directory = sdsnew(MYMPD_BUILD_DIR"/testfiles");
    struct t_lyrics lyrics = {
        .sylt_ext = sdsnew(MYMPD_LYRICS_SYLT_EXT),
        .uslt_ext = sdsnew(MYMPD_LYRICS_USLT_EXT),
        .vorbis_sylt = sdsnew(MYMPD_LYRICS_VORBIS_SYLT),
        .vorbis_uslt = sdsnew(MYMPD_LYRICS_VORBIS_USLT)
    };
    //id3v2
    //testfile has synced and unsynced lyrics
    sds uri = sdsnew("test.mp3");
    sds buffer = mympd_api_lyrics_get(&lyrics, music_directory, sdsempty(), 0, uri);
    int result = 0;
    json_get_int(buffer, "$.result.returnedEntities", 0, 20, &result, NULL);
    ASSERT_EQ(4, result);

    sdsclear(buffer);
    sdsclear(uri);

    sdsfree(uri);
    sdsfree(buffer);
    sdsfree(music_directory);
    sdsfree(lyrics.sylt_ext);
    sdsfree(lyrics.uslt_ext);
    sdsfree(lyrics.vorbis_sylt);
    sdsfree(lyrics.vorbis_uslt);
}
