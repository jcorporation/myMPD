/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2024 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include "compile_time.h"
#include "utility.h"

#include "dist/utest/utest.h"
#include "src/lib/mimetype.h"

#include <stdio.h>

UTEST(mimetype, test_get_mime_type_by_ext) {
    const char *mime_type = get_mime_type_by_ext("test.mp3");
    ASSERT_STREQ("audio/mpeg", mime_type);

    mime_type = get_mime_type_by_ext("test.unknown");
    ASSERT_STREQ("application/octet-stream", mime_type);

    mime_type = get_mime_type_by_ext("/tes/testwoext");
    ASSERT_STREQ("application/octet-stream", mime_type);

    mime_type = get_mime_type_by_ext("");
    ASSERT_STREQ("application/octet-stream", mime_type);
}

UTEST(mimetype, test_get_ext_by_mime_type) {
    const char *ext = get_ext_by_mime_type("audio/ogg");
    ASSERT_STREQ("oga", ext);

    ext = get_ext_by_mime_type("application/octet-stream");
    ASSERT_TRUE(ext == NULL);

    ext = get_ext_by_mime_type("");
    ASSERT_TRUE(ext == NULL);
}

UTEST(mimetype, test_get_mime_type_by_magic_stream) {
    FILE *fp = fopen(MYMPD_BUILD_DIR"/testfiles/back.jpg", "rb");
    ASSERT_FALSE(fp == NULL);

    unsigned char binary_buffer[12];
    size_t read = fread(binary_buffer, 1, sizeof(binary_buffer), fp);
    fclose(fp);
    sds stream = sdsnewlen(binary_buffer, read);
    const char *mime_type = get_mime_type_by_magic_stream(stream);
    ASSERT_STREQ("image/jpeg", mime_type);

    //empty
    sdsclear(stream);
    mime_type = get_mime_type_by_magic_stream(stream);
    ASSERT_STREQ("application/octet-stream", mime_type);
    sdsfree(stream);
}

UTEST(mimetype, test_get_mime_type_by_magic_file) {
    const char *mime_type = get_mime_type_by_magic_file(MYMPD_BUILD_DIR"/testfiles/back.jpg");
    ASSERT_STREQ("image/jpeg", mime_type);
}

UTEST(mimetype, test_is_image) {
    ASSERT_TRUE(is_image("image.png"));
    ASSERT_FALSE(is_image(""));
    ASSERT_FALSE(is_image("test.mp3"));
}
