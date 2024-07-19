/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2024 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include "compile_time.h"
#include "utility.h"

#include "dist/utest/utest.h"
#include "src/lib/mympd_state.h"

UTEST(mympd_state, test_copy_tag_types) {
    struct t_mpd_tags src_taglist;
    struct t_mpd_tags dst_taglist;
    mpd_tags_reset(&src_taglist);
    mpd_tags_reset(&dst_taglist);

    src_taglist.tags[0] = MPD_TAG_ALBUM;
    src_taglist.tags[1] = MPD_TAG_ALBUM_ARTIST;
    src_taglist.len = 2;

    mpd_tags_clone(&src_taglist, &dst_taglist);

    ASSERT_EQ(MPD_TAG_ALBUM, dst_taglist.tags[0]);
    ASSERT_EQ(MPD_TAG_ALBUM_ARTIST, dst_taglist.tags[1]);

}

UTEST(mympd_state, test_mpd_state_copy) {
    struct t_mpd_state *src = malloc(sizeof(struct t_mpd_state));
    mpd_state_default(src, NULL);

    struct t_mpd_state *dst = malloc(sizeof(struct t_mpd_state));
    mpd_state_copy(src, dst);
    ASSERT_EQ(6600U, dst->mpd_port);

    mpd_state_free(src);
    mpd_state_free(dst);
}

UTEST(mympd_state, test_mpd_state_features_copy) {
    struct t_mpd_features src;
    mpd_state_features_default(&src);
    src.albumart = true;

    struct t_mpd_features dst;
    mpd_state_features_copy(&src, &dst);
    ASSERT_TRUE(dst.albumart);
    ASSERT_FALSE(dst.advqueue);

    mpd_state_features_default(&src);
    ASSERT_FALSE(src.albumart);
}
