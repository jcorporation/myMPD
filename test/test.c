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
#include <stdbool.h>

#include "../dist/src/sds/sds.h"
#include "../src/sds_extras.h"
#include "../src/tiny_queue.h"
#include "../src/list.h"

_Thread_local sds thread_logname;

int main(void) {
//tests tiny queue
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
    
//test list
    struct list *test_list = (struct list *) malloc(sizeof(struct list));
    assert(test_list);
    list_init(test_list);
    list_push(test_list, "key1", 1, "value1", NULL);
    list_push(test_list, "key2", 2, "value2", NULL);
    list_push(test_list, "key3", 3, "value3", NULL);
    list_push(test_list, "key4", 4, "value4", NULL);
    list_push(test_list, "key5", 5, "value5", NULL);
    list_insert(test_list, "key0", 0, "value0", NULL);
    //test1
    list_swap_item_pos(test_list, 3, 1);
    list_move_item_pos(test_list, 4, 2);
    //remove middle item
    list_shift(test_list,3);
    //remove last item
    list_shift(test_list, 5);
    //remove first item
    list_shift(test_list, 0);
    list_push(test_list, "key6", 6, "value6", NULL);
    list_insert(test_list, "key7", 7, "value7", NULL);
    int i = 0;
    struct list_node *current = test_list->head;
    while (current != NULL) {
        printf("%d: %s\n", i, current->key);
        current = current->next;
        i++;
    }
    list_free(test_list);
    free(test_list);
}
