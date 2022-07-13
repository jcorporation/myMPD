/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2022 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include "mympd_config_defs.h"

#include "../../dist/utest/utest.h"
#include "../../src/lib/mympd_queue.h"
#include "../../src/lib/sds_extras.h"

UTEST(mympd_queue, push_shift) {
    struct t_mympd_queue *test_queue = mympd_queue_create("test");
    sds test_data_in0 = sdsnew("test0");
    sds test_data_in1 = sdsnew("test0");
    sds test_data_in2 = sdsnew("test0");

    sds test_data_out = NULL;

    mympd_queue_push(test_queue, test_data_in0, 0);
    test_data_out = mympd_queue_shift(test_queue, 50, 0);
    ASSERT_STREQ(test_data_in0, test_data_out);

    long len = mympd_queue_length(test_queue, 0);
    ASSERT_EQ(0, len);

    mympd_queue_push(test_queue, test_data_in1, 0);
    mympd_queue_push(test_queue, test_data_in2, 0);
    test_data_out = NULL;
    test_data_out = mympd_queue_shift(test_queue, 50, 0);
    ASSERT_STREQ(test_data_in1, test_data_out);

    test_data_out = NULL;
    test_data_out = mympd_queue_shift(test_queue, 50, 0);
    ASSERT_STREQ(test_data_in2, test_data_out);

    len = mympd_queue_length(test_queue, 0);
    ASSERT_EQ(0, len);

    mympd_queue_free(test_queue);
    sdsfree(test_data_in0);
    sdsfree(test_data_in1);
    sdsfree(test_data_in2);
}

UTEST(mympd_queue, push_shift_id) {
    struct t_mympd_queue *test_queue = mympd_queue_create("test");
    sds test_data_in0 = sdsnew("test0");
    sds test_data_in1 = sdsnew("test0");
    sds test_data_in2 = sdsnew("test0");

    sds test_data_out = NULL;

    mympd_queue_push(test_queue, test_data_in0, 10);
    mympd_queue_push(test_queue, test_data_in1, 20);
    mympd_queue_push(test_queue, test_data_in2, 10);

    long len = mympd_queue_length(test_queue, 0);
    ASSERT_EQ(3, len);

    test_data_out = NULL;
    test_data_out = mympd_queue_shift(test_queue, 50, 20);
    ASSERT_STREQ(test_data_in1, test_data_out);

    test_data_out = NULL;
    test_data_out = mympd_queue_shift(test_queue, 50, 10);
    ASSERT_STREQ(test_data_in0, test_data_out);

    test_data_out = NULL;
    test_data_out = mympd_queue_shift(test_queue, 50, 10);
    ASSERT_STREQ(test_data_in2, test_data_out);

    len = mympd_queue_length(test_queue, 0);
    ASSERT_EQ(0, len);

    mympd_queue_free(test_queue);
    sdsfree(test_data_in0);
    sdsfree(test_data_in1);
    sdsfree(test_data_in2);
}
