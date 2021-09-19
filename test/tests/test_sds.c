/*
 SPDX-License-Identifier: GPL-2.0-or-later
 myMPD (c) 2018-2021 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include "mympd_config_defs.h"
#include "test_sds.h"

#include "../../src/lib/sds_extras.h"

#include <stdio.h>
#include <stdlib.h>

//private

static const char *filenames[] = {
    "/test/test.mp3",
    "/test/withoutextension",
    "",
    "/test/test.mp3.mp3",
    NULL
};

static const char *uris[] = {
    "http://host:port/verz/verz/test?safsaf#798234",
    "http://host:port/verz/verz/test?safsaf#798234",
    "",
    "/test/test.mp3.mp3",
    NULL
};

static void test_sds_strip_file_extension() {
    const char **p = filenames;
    sds testfilename = sdsempty();
    while (*p != NULL) {
        testfilename = sdscatfmt(testfilename, "%s", *p);
        printf("Before sds_strip_file_extension: %s\n", testfilename);
        sds_strip_file_extension(testfilename);
        printf("After sds_strip_file_extension: %s\n", testfilename);
        sdsclear(testfilename);
        p++;
    }
    sdsfree(testfilename);
}

static void test_sds_streamuri_to_filename(void) {
    const char **p = uris;
    sds testfilename = sdsempty();
    while (*p != NULL) {
        testfilename = sdscatfmt(testfilename, "%s", *p);
        printf("Before sds_streamuri_to_filename: %s\n", testfilename);
        sds_streamuri_to_filename(testfilename);
        printf("After sds_streamuri_to_filename: %s\n", testfilename);
        sdsclear(testfilename);
        p++;
    }
    sdsfree(testfilename);
}

static void test_sds_get_extension_from_filename(void) {
    const char **p = filenames;
    while (*p != NULL) {
        printf("Filename: %s\n", *p);
        sds ext = sds_get_extension_from_filename(*p);
        printf("Extension: %s\n", ext);
        sdsfree(ext);
        p++;
    }
}

static void test_sds_strip_slash(void) {
    const char **p = uris;
    sds testfilename = sdsempty();
    while (*p != NULL) {
        testfilename = sdscatfmt(testfilename, "%s", *p);
        printf("Before sds_strip_slash: %s\n", testfilename);
        sds_streamuri_to_filename(testfilename);
        printf("After sds_strip_slash: %s\n", testfilename);
        sdsclear(testfilename);
        p++;
    }
    sdsfree(testfilename);
}

static void test_sds_basename_uri(void) {
    const char **p = uris;
    sds testfilename = sdsempty();
    while (*p != NULL) {
        testfilename = sdscatfmt(testfilename, "%s", *p);
        printf("Before sds_basename_uri: %s\n", testfilename);
        sds_basename_uri(testfilename);
        printf("After sds_basename_uri: %s\n", testfilename);
        sdsclear(testfilename);
        p++;
    }
    sdsfree(testfilename);
}

//public
void test_sds(void) {
    test_sds_strip_file_extension();
    test_sds_streamuri_to_filename();
    test_sds_get_extension_from_filename();
    test_sds_strip_slash();
    test_sds_basename_uri();
}
