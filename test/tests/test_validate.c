/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2023 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include "compile_time.h"

#include "dist/utest/utest.h"
#include "src/lib/validate.h"

UTEST(validate, test_validate_json) {
    //valid
    sds data = sdsnew("{\"key1\":\"value1\"}");
    ASSERT_TRUE(validate_json_object(data));
    sdsclear(data);
    //invalid
    data = sdscat(data, "asdfsfdjlk");
    ASSERT_FALSE(validate_json_object(data));
    sdsfree(data);
}

UTEST(validate, test_validate_array) {
    //valid
    sds data = sdsnew("[\"key1\":\"value1\"]");
    ASSERT_TRUE(validate_json_array(data));
    sdsclear(data);
    //invalid
    data = sdscat(data, "asdfsfdjlk");
    ASSERT_FALSE(validate_json_array(data));
    sdsfree(data);
}

UTEST(validate, test_validate_isalnum) {
    //valid
    sds data = sdsnew("asdfdsf12312_");
    ASSERT_TRUE(vcb_isalnum(data));
    sdsclear(data);
    //invalid
    data = sdscat(data, "asdfsfdjl/k");
    ASSERT_FALSE(vcb_isalnum(data));
    sdsfree(data);
}

UTEST(validate, test_validate_isdigit) {
    //valid
    sds data = sdsnew("012312");
    ASSERT_TRUE(vcb_isdigit(data));
    sdsclear(data);
    //invalid
    data = sdscat(data, "asdfsfdjl/k");
    ASSERT_FALSE(vcb_isdigit(data));
    sdsfree(data);
}

UTEST(validate, test_validate_isprint) {
    //valid
    sds data = sdsnew("012312");
    ASSERT_TRUE(vcb_isprint(data));
    sdsclear(data);
    //invalid
    data = sdscat(data, "asdfsfdjl/köä");
    ASSERT_FALSE(vcb_isprint(data));
    sdsfree(data);
}

UTEST(validate, test_validate_ishexcolor) {
    //valid
    sds data = sdsnew("#ffbbcc");
    ASSERT_TRUE(vcb_ishexcolor(data));
    sdsclear(data);
    //invalid
    data = sdscat(data, "asdfsfdjl/köä");
    ASSERT_FALSE(vcb_ishexcolor(data));
    sdsclear(data);
    data = sdscat(data, "ffbb#aa");
    ASSERT_FALSE(vcb_ishexcolor(data));
    sdsfree(data);
}

UTEST(validate, test_validate_isname) {
    //valid
    sds data = sdsnew("01231wer23-äö?2");
    ASSERT_TRUE(vcb_isname(data));
    sdsclear(data);
    //invalid
    data = sdscat(data, "asdfsfdjl\n");
    ASSERT_FALSE(vcb_isname(data));
    sdsclear(data);
    data = sdscat(data, "sdaf\\u4589");
    ASSERT_FALSE(vcb_isname(data));
    sdsclear(data);
    data = sdscat(data, "sdaf\a4589");
    ASSERT_FALSE(vcb_isname(data));
    sdsfree(data);
}

UTEST(validate, test_validate_istext) {
    //valid
    sds data = sdsnew("01231wer23-äö\n?2");
    ASSERT_TRUE(vcb_istext(data));
    sdsclear(data);
    //invalid
    data = sdscat(data, "asdfsfdjl\a");
    ASSERT_FALSE(vcb_istext(data));
    sdsfree(data);
}

UTEST(validate, test_validate_isfilename) {
    //valid
    sds data = sdsnew("01231wer23-äö2");
    ASSERT_TRUE(vcb_isfilename(data));
    sdsclear(data);
    //invalid
    data = sdscat(data, "asdfsfd\ajl");
    ASSERT_FALSE(vcb_isfilename(data));
    sdsclear(data);
    data = sdscat(data, "asdfsfd/jl");
    ASSERT_FALSE(vcb_isfilename(data));
    sdsfree(data);
}

UTEST(validate, test_validate_isfilepath) {
    //valid
    sds data = sdsnew("01231wer23-äö2");
    ASSERT_TRUE(vcb_isfilepath(data));
    sdsclear(data);
    //invalid
    data = sdscat(data, "asdfs\bfdjl");
    ASSERT_FALSE(vcb_isfilepath(data));
    sdsclear(data);
    data = sdscat(data, "asdfsfd\\u5676jl");
    ASSERT_FALSE(vcb_isfilepath(data));
    sdsclear(data);
    data = sdscatlen(data, "asdfsfd\0jl", 10);
    ASSERT_FALSE(vcb_isfilepath(data));
    sdsfree(data);
}

UTEST(validate, test_validate_isuri) {
    //valid
    sds data = sdsnew("https://jcgames.de");
    ASSERT_TRUE(vcb_isuri(data));
    sdsclear(data);
    data = sdscat(data, "/asdfsfdjl/sdf");
    ASSERT_TRUE(vcb_isuri(data));
    sdsfree(data);
}

UTEST(validate, test_validate_iscolumn) {
    //valid
    sds data = sdsnew("Artist");
    ASSERT_TRUE(vcb_iscolumn(data));
    sdsclear(data);
    data = sdscat(data, "Duration");
    ASSERT_TRUE(vcb_iscolumn(data));
    sdsclear(data);
    //invalid
    data = sdscat(data, "asdfsfdj");
    ASSERT_FALSE(vcb_iscolumn(data));
    sdsfree(data);
}

UTEST(validate, test_validate_istaglist) {
    //valid
    sds data = sdsnew("Artist, Title");
    ASSERT_TRUE(vcb_istaglist(data));
    sdsclear(data);
    //invalid
    data = sdscat(data, "Artist, asdfsfdj");
    ASSERT_FALSE(vcb_istaglist(data));
    sdsfree(data);
}

UTEST(validate, test_validate_ismpdtag) {
    //valid
    sds data = sdsnew("Artist");
    ASSERT_TRUE(vcb_ismpdtag(data));
    sdsclear(data);
    //invalid
    data = sdscat(data, "asdfsfdj");
    ASSERT_FALSE(vcb_ismpdtag(data));
    sdsfree(data);
}

UTEST(validate, test_validate_ismpdtag_or_any) {
    //valid
    sds data = sdsnew("Artist");
    ASSERT_TRUE(vcb_ismpdtag_or_any(data));
    sdsclear(data);
    data = sdscat(data, "any");
    ASSERT_TRUE(vcb_ismpdtag_or_any(data));
    sdsclear(data);
    //invalid
    data = sdscat(data, "asdfsfdj");
    ASSERT_FALSE(vcb_ismpdtag_or_any(data));
    sdsfree(data);
}

UTEST(validate, test_validate_ismpdsort) {
    //valid
    sds data = sdsnew("Artist");
    ASSERT_TRUE(vcb_ismpdsort(data));
    sdsclear(data);
    data = sdscat(data, "shuffle");
    ASSERT_TRUE(vcb_ismpdsort(data));
    sdsclear(data);
    //invalid
    data = sdscat(data, "asdfsfdj");
    ASSERT_FALSE(vcb_ismpdsort(data));
    sdsfree(data);
}
