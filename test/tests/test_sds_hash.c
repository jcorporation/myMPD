/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2026 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include "compile_time.h"
#include "utility.h"

#include "dist/utest/utest.h"
#include "src/lib/sds/sds_hash.h"

UTEST(sds_hash, test_sds_hash_md5) {
    sds hash = sds_hash_md5("abc");
    ASSERT_STREQ("900150983cd24fb0d6963f7d28e17f72", hash);
    sdsfree(hash);
}

UTEST(sds_hash, test_sds_hash_sha1) {
    sds hash = sds_hash_sha1("abc");
    ASSERT_STREQ("a9993e364706816aba3e25717850c26c9cd0d89d", hash);
    sdsfree(hash);
}

UTEST(sds_hash, test_sds_hash_sha1_sds) {
    sds hash = sdsnew("abc");
    hash = sds_hash_sha1_sds(hash);
    ASSERT_STREQ("a9993e364706816aba3e25717850c26c9cd0d89d", hash);
    sdsfree(hash);
}

UTEST(sds_hash, test_sds_hash_sha256) {
    sds hash = sds_hash_sha256("abc");
    ASSERT_STREQ("ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad", hash);
    sdsfree(hash);
}

UTEST(sds_hash, test_sds_hash_sha256_sds) {
    sds hash = sdsnew("abc");
    hash = sds_hash_sha256_sds(hash);
    ASSERT_STREQ("ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad", hash);
    sdsfree(hash);
}
