/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2022 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include "mympd_config_defs.h"

#include "../../dist/rax/rax.h"
#include "../../src/lib/list.h"
#include "../../src/lib/sds_extras.h"

#include <time.h>

#define MEASURE_INIT struct timespec tic, toc;
#define MEASURE_START clock_gettime(CLOCK_MONOTONIC, &tic);
#define MEASURE_END clock_gettime(CLOCK_MONOTONIC, &toc);
#define MEASURE_PRINT(X) printf("Execution time for %s: %lld ms\n", X, ((long long)(toc.tv_sec) * 1000 + toc.tv_nsec / 1000000) - ((long long)(tic.tv_sec) * 1000 + tic.tv_nsec / 1000000));

_Thread_local sds thread_logname;

int main(int argc, const char *const argv[]) {
    thread_logname = sdsempty();
    struct t_list song_list;
    list_init(&song_list);

    if (argc != 2) {
        printf("Usage: benchmark <file>\n");
        goto end;
    }

    //create a large list with random order
    FILE *fp = fopen(argv[1], "r");
    if (fp == NULL) {
        printf("Error opening \"%s\"\n", argv[1]);
        goto end;
    }
    sds line = sdsempty();
    MEASURE_INIT
    MEASURE_START
    while (sds_getline(&line, fp, 10000) == 0) {
        list_push(&song_list, line, 0, NULL, NULL);
    }
    MEASURE_END
    MEASURE_PRINT("Populating list")
    printf("List length: %ld\n", song_list.length);
    fclose(fp);
    sdsfree(line);
    MEASURE_START
    list_shuffle(&song_list);
    MEASURE_END
    MEASURE_PRINT("Shuffling list")

    struct t_list sorted_list;
    list_init(&sorted_list);
    struct t_list_node *current;

    //create full sorted list
    /*
    MEASURE_START
    current = song_list.head;
    while (current != NULL) {
        list_insert_sorted_by_key(&sorted_list, current->key, 0, NULL, NULL, LIST_SORT_ASC);
        current = current->next;
    }
    list_clear(&sorted_list);
    MEASURE_END
    MEASURE_PRINT("Create new sorted list")
    */

    //create limited sorted list
    MEASURE_START
    current = song_list.head;
    while (current != NULL) {
        list_insert_sorted_by_key_limit(&sorted_list, current->key, 0, NULL, NULL, LIST_SORT_ASC, 500, NULL);
        current = current->next;
    }
    list_clear(&sorted_list);
    MEASURE_END
    MEASURE_PRINT("Create new limited sorted list (500)")

    MEASURE_START
    current = song_list.head;
    while (current != NULL) {
        list_insert_sorted_by_key_limit(&sorted_list, current->key, 0, NULL, NULL, LIST_SORT_ASC, 1000, NULL);
        current = current->next;
    }
    list_clear(&sorted_list);
    MEASURE_END
    MEASURE_PRINT("Create new limited sorted list (1000)")

    //create rax
    rax *r = raxNew();
    MEASURE_START
    current = song_list.head;
    while (current != NULL) {
        raxInsert(r, (unsigned char *)current->key, sdslen(current->key), NULL, NULL);
        current = current->next;
    }
    raxFree(r);
    MEASURE_END
    MEASURE_PRINT("Create new sorted rax tree")

    end:
    list_clear(&song_list);
    FREE_SDS(thread_logname);
    return 0;
}
