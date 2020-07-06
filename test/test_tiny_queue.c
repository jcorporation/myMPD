/*
 SPDX-License-Identifier: GPL-2.0-or-later
 myMPD (c) 2018-2020 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <signal.h>
#include <assert.h>
#include <string.h>

#include "../dist/src/sds/sds.h"
#include "../src/tiny_queue.h"

_Thread_local sds thread_logname;

int main(void) {
    thread_logname = sdsempty();
    tiny_queue_t *test_queue = tiny_queue_create();
    sds test_data_in0 = sdsnew("test0");
    sds test_data_in1 = sdsnew("test0");
    sds test_data_in2 = sdsnew("test0");
    
    sds test_data_out;
    
    //test1
    tiny_queue_push(test_queue, test_data_in0, 0);
    test_data_out = tiny_queue_shift(test_queue, 50, 0);
    printf(strcmp(test_data_out, test_data_in0) == 0 ? "OK\n" : "ERROR\n");
    
    //test2
    tiny_queue_push(test_queue, test_data_in1, 0);
    tiny_queue_push(test_queue, test_data_in2, 0);
    test_data_out = NULL;
    test_data_out = tiny_queue_shift(test_queue, 50, 0);
    printf(strcmp(test_data_out, test_data_in1) == 0 ? "OK\n" : "ERROR\n");

    test_data_out = NULL;
    test_data_out = tiny_queue_shift(test_queue, 50, 0);
    printf(strcmp(test_data_out, test_data_in2) == 0 ? "OK\n" : "ERROR\n");
    
    //test3
    tiny_queue_push(test_queue, test_data_in0, 10);
    tiny_queue_push(test_queue, test_data_in1, 20);
    tiny_queue_push(test_queue, test_data_in2, 10);
    test_data_out = NULL;
    test_data_out = tiny_queue_shift(test_queue, 50, 20);
    printf(strcmp(test_data_out, test_data_in1) == 0 ? "OK\n" : "ERROR\n");

    test_data_out = NULL;
    test_data_out = tiny_queue_shift(test_queue, 50, 10);
    printf(strcmp(test_data_out, test_data_in0) == 0 ? "OK\n" : "ERROR\n");
    
    test_data_out = NULL;
    test_data_out = tiny_queue_shift(test_queue, 50, 10);
    printf(strcmp(test_data_out, test_data_in2) == 0 ? "OK\n" : "ERROR\n");

    tiny_queue_free(test_queue);
    sdsfree(thread_logname);
    sdsfree(test_data_in0);
    sdsfree(test_data_in1);
    sdsfree(test_data_in2);
}
