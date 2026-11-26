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

UTEST(sds_hash, test_sds_hash_empty) {
    sds md5 = sds_hash_md5("");
    ASSERT_EQ((size_t)32, sdslen(md5));
    ASSERT_STREQ("d41d8cd98f00b204e9800998ecf8427e", md5);
    sdsfree(md5);

    sds sha1 = sds_hash_sha1("");
    ASSERT_EQ((size_t)40, sdslen(sha1));
    ASSERT_STREQ("da39a3ee5e6b4b0d3255bfef95601890afd80709", sha1);
    sdsfree(sha1);

    sds sha256 = sds_hash_sha256("");
    ASSERT_EQ((size_t)64, sdslen(sha256));
    ASSERT_STREQ("e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855", sha256);
    sdsfree(sha256);
}

UTEST(sds_hash, test_sds_hash_binary_sds) {
    sds sha1 = sdsnewlen("a\0c", 3);
    sha1 = sds_hash_sha1_sds(sha1);
    ASSERT_EQ((size_t)40, sdslen(sha1));
    ASSERT_STREQ("18a68aca12cdd2045a1a4d9a13893577e8bbbc96", sha1);
    sdsfree(sha1);

    sds sha256 = sdsnewlen("a\0c", 3);
    sha256 = sds_hash_sha256_sds(sha256);
    ASSERT_EQ((size_t)64, sdslen(sha256));
    ASSERT_STREQ("b179f49283f586ae1922b625af4017aecdf77e7ee175c0af675abea14252a666", sha256);
    sdsfree(sha256);
}
