/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2025 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include "compile_time.h"
#include "utility.h"

#include "dist/utest/utest.h"
#include "src/lib/config.h"
#include "src/lib/jsonrpc.h"
#include "src/lib/mympd_state.h"
#include "src/mympd_api/lyrics.h"

UTEST(lyrics_id3, test_mympd_api_lyrics_get) {
    struct t_config *config = malloc(sizeof(struct t_config));
    mympd_config_defaults_initial(config);
    mympd_config_defaults(config);
    struct t_mympd_state *mympd_state = malloc(sizeof(struct t_mympd_state));
    mympd_state_default(mympd_state, config);
    mympd_state->mpd_state->music_directory_value = sdscat(mympd_state->mpd_state->music_directory_value, MYMPD_BUILD_DIR"/testfiles");

    //flac
    //testfile has unsynced lyrics
    sds uri = sdsnew("test.mp3");
    sds partition = sdsnew("default");
    sds buffer = mympd_api_lyrics_get(mympd_state, sdsempty(), uri, partition, 0, 0);
    int result = 0;
    json_get_int(buffer, "$.result.returnedEntities", 0, 20, &result, NULL);
    ASSERT_EQ(4, result);
    
    sdsfree(uri);
    sdsfree(partition);
    sdsfree(buffer);
    mympd_state_free(mympd_state);
    mympd_config_free(config);
}
