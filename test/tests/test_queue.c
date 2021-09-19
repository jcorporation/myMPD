/*
 SPDX-License-Identifier: GPL-2.0-or-later
 myMPD (c) 2018-2021 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include "mympd_config_defs.h"

#include "../../src/lib/mympd_queue.h"
#include "../../src/lib/sds_extras.h"

#include "test_queue.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void test_queue(void) {
    struct t_mympd_queue *test_queue = mympd_queue_create("test");
    sds test_data_in0 = sdsnew("test0");
    sds test_data_in1 = sdsnew("test0");
    sds test_data_in2 = sdsnew("test0");
    
    sds test_data_out;
    
    //test1
    mympd_queue_push(test_queue, test_data_in0, 0);
    test_data_out = mympd_queue_shift(test_queue, 50, 0);
    printf(strcmp(test_data_out, test_data_in0) == 0 ? "OK\n" : "ERROR\n");
    
    //test2
    mympd_queue_push(test_queue, test_data_in1, 0);
    mympd_queue_push(test_queue, test_data_in2, 0);
    test_data_out = NULL;
    test_data_out = mympd_queue_shift(test_queue, 50, 0);
    printf(strcmp(test_data_out, test_data_in1) == 0 ? "OK\n" : "ERROR\n");

    test_data_out = NULL;
    test_data_out = mympd_queue_shift(test_queue, 50, 0);
    printf(strcmp(test_data_out, test_data_in2) == 0 ? "OK\n" : "ERROR\n");
    
    //test3
    mympd_queue_push(test_queue, test_data_in0, 10);
    mympd_queue_push(test_queue, test_data_in1, 20);
    mympd_queue_push(test_queue, test_data_in2, 10);
    test_data_out = NULL;
    test_data_out = mympd_queue_shift(test_queue, 50, 20);
    printf(strcmp(test_data_out, test_data_in1) == 0 ? "OK\n" : "ERROR\n");

    test_data_out = NULL;
    test_data_out = mympd_queue_shift(test_queue, 50, 10);
    printf(strcmp(test_data_out, test_data_in0) == 0 ? "OK\n" : "ERROR\n");
    
    test_data_out = NULL;
    test_data_out = mympd_queue_shift(test_queue, 50, 10);
    printf(strcmp(test_data_out, test_data_in2) == 0 ? "OK\n" : "ERROR\n");

    mympd_queue_free(test_queue);
    sdsfree(test_data_in0);
    sdsfree(test_data_in1);
    sdsfree(test_data_in2);
}
