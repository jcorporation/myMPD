/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2023 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include "compile_time.h"

#include "dist/utest/utest.h"
#include "src/lib/mympd_state.h"

UTEST(mympd_state, test_copy_tag_types) {
    struct t_tags src_taglist;
    struct t_tags dst_taglist;
    reset_t_tags(&src_taglist);
    reset_t_tags(&dst_taglist);

    src_taglist.tags[0] = MPD_TAG_ALBUM;
    src_taglist.tags[0] = MPD_TAG_ALBUM_ARTIST;
    src_taglist.len = 2;

    copy_tag_types(src_taglist, dst_taglist);

    ASSERT_EQ(MPD_TAG_ALBUM, dst_taglist.tags[0]);
    ASSERT_EQ(MPD_TAG_ALBUM_ARTIST, dst_taglist.tags[1]);

}
