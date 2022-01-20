/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2022 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include "mympd_config_defs.h"
#include "list.h"

#include "mem.h"
#include "random.h"
#include "sds_extras.h"

#include <stdlib.h>
#include <string.h>

//private definitions
static struct t_list_node *list_node_extract(struct t_list *l, long idx);

//public functions

//Mallocs a new list and inits it
struct t_list *list_new(void) {
    struct t_list *l = malloc_assert(sizeof(struct t_list));
    list_init(l);
    return l;
}

//Inits a already allocated list
void list_init(struct t_list *l) {
    l->length = 0;
    l->head = NULL;
    l->tail = NULL;
}

//Clears the list and frees all nodes and there values
void list_clear(struct t_list *l) {
    list_clear_user_data(l, NULL);
}

void list_clear_user_data(struct t_list *l, user_data_callback free_cb) {
    struct t_list_node *current = l->head;
    struct t_list_node *tmp = NULL;
    while (current != NULL) {
        FREE_SDS(current->key);
        FREE_SDS(current->value_p);
        if (current->user_data != NULL && free_cb != NULL) {
            //callback to free user_data
            free_cb(current);
        }
        else if (free_cb == NULL) {
            FREE_PTR(current->user_data);
        }
        tmp = current;
        current = current->next;
        free(tmp);
    }
    list_init(l);
}

void list_free_cb_ignore_user_data(struct t_list_node *current) {
    //remove user_data pointer
    current->user_data = NULL;
}

long list_get_value_i(const struct t_list *l, const char *key) {
    long value_i = -1;
    struct t_list_node *current = l->head;
    while (current != NULL) {
        if (strcmp(current->key, key) == 0) {
            value_i = current->value_i;
            break;
        }
        current = current->next;
    }
    return value_i;
}

sds list_get_value_p(const struct t_list *l, const char *key) {
    sds value_p = NULL;
    struct t_list_node *current = l->head;
    while (current != NULL) {
        if (strcmp(current->key, key) == 0) {
            value_p = current->value_p;
            break;
        }
        current = current->next;
    }
    return value_p;
}

void *list_get_user_data(const struct t_list *l, const char *key) {
    void *user_data = NULL;
    struct t_list_node *current = l->head;
    while (current != NULL) {
        if (strcmp(current->key, key) == 0) {
            user_data = current->user_data;
            break;
        }
        current = current->next;
    }
    return user_data;
}

struct t_list_node *list_get_node(const struct t_list *l, const char *key) {
    struct t_list_node *current = l->head;
    while (current != NULL) {
        if (strcmp(current->key, key) == 0) {
            break;
        }
        current = current->next;
    }
    return current;
}

struct t_list_node *list_node_at(const struct t_list *l, long index) {
    //if there's no data in the list, fail
    if (l->head == NULL) {
        return NULL;
    }
    struct t_list_node * current = l->head;
    for (; index > 0; index--) {
        if (current->next == NULL) {
            return NULL;
        }
        current = current->next;
    }
    return current;
}

bool list_move_item_pos(struct t_list *l, long from, long to) {
    if (from > l->length || to > l->length) {
        return false;
    }
    if (from == to) {
        return true;
    }

    //extract node at from position;
    struct t_list_node *node = list_node_extract(l, from);
    if (node == NULL) {
        return false;
    }

    struct t_list_node *current = l->head;
    struct t_list_node **previous = &l->head;
    if (to > from) {
        to--;
    }
    for (; to > 0; to--) {
        previous = &current->next;
        current = current->next;
    }
    //insert extracted node
    node->next = *previous;
    *previous = node;
    l->length++;
    return true;
}

bool list_swap_item_pos(struct t_list *l, long index1, long index2) {
    if (l->length < 2) {
        return false;
    }
    struct t_list_node *node1 = list_node_at(l, index1);
    struct t_list_node *node2 = list_node_at(l, index2);
    if (node1 == NULL || node2 == NULL) {
        return false;
    }
    return list_swap_item(node1, node2);
}

bool list_swap_item(struct t_list_node *n1, struct t_list_node *n2) {
    if (n1 == n2) {
        return false;
    }

    if (n1 == NULL || n2 == NULL) {
        return false;
    }

    sds key = n2->key;
    long value_i = n2->value_i;
    sds value_p = n2->value_p;
    void *user_data = n2->user_data;

    n2->key = n1->key;
    n2->value_i = n1->value_i;
    n2->value_p = n1->value_p;
    n2->user_data = n1->user_data;

    n1->key = key;
    n1->value_i = value_i;
    n1->value_p = value_p;
    n1->user_data = user_data;

    return true;
}

bool list_shuffle(struct t_list *l) {
    if (l->length < 2) {
        return false;
    }
    int n = 0;
    struct t_list_node *current = l->head;
    while (current != NULL) {
        long pos = randrange(0, l->length);
        list_swap_item(current, list_node_at(l, pos));
        n++;
        current = current->next;
    }
    return true;
}

bool list_sort_by_value_i(struct t_list *l, enum list_sort_direction direction) {
    int swapped;
    struct t_list_node *ptr1;
    struct t_list_node *lptr = NULL;

    if (l->head == NULL) {
        return false;
    }

    do {
        swapped = 0;
        ptr1 = l->head;

        while (ptr1->next != lptr) {
            if ((direction == LIST_SORT_ASC && ptr1->value_i > ptr1->next->value_i) ||
                (direction == LIST_SORT_DESC && ptr1->value_i < ptr1->next->value_i))
            {
                list_swap_item(ptr1, ptr1->next);
                swapped = 1;
            }
            ptr1 = ptr1->next;
        }
        lptr = ptr1;
    }
    while (swapped);
    return true;
}

bool list_sort_by_value_p(struct t_list *l, enum list_sort_direction direction) {
    int swapped;
    struct t_list_node *ptr1;
    struct t_list_node *lptr = NULL;

    if (l->head == NULL) {
        return false;
    }

    do {
        swapped = 0;
        ptr1 = l->head;

        while (ptr1->next != lptr) {
            if ((direction == LIST_SORT_ASC && strcmp(ptr1->value_p, ptr1->next->value_p) > 0) ||
                (direction == LIST_SORT_DESC && strcmp(ptr1->value_p, ptr1->next->value_p) < 0))
            {
                list_swap_item(ptr1, ptr1->next);
                swapped = 1;
            }
            ptr1 = ptr1->next;
        }
        lptr = ptr1;
    }
    while (swapped);
    return true;
}

bool list_sort_by_key(struct t_list *l, enum list_sort_direction direction) {
    int swapped;
    struct t_list_node *ptr1;
    struct t_list_node *lptr = NULL;

    if (l->head == NULL) {
        return false;
    }

    do {
        swapped = 0;
        ptr1 = l->head;

        while (ptr1->next != lptr) {
            if ((direction == LIST_SORT_ASC && strcmp(ptr1->key, ptr1->next->key) > 0) ||
                (direction == LIST_SORT_DESC && strcmp(ptr1->key, ptr1->next->key) < 0))
            {
                list_swap_item(ptr1, ptr1->next);
                swapped = 1;
            }
            ptr1 = ptr1->next;
        }
        lptr = ptr1;
    }
    while (swapped);
    return true;
}

bool list_replace(struct t_list *l, long pos, const char *key, long value_i, const char *value_p, void *user_data) {
    if (pos >= l->length) {
        return false;
    }
    long i = 0;
    struct t_list_node *current = l->head;
    while (current->next != NULL) {
        if (i == pos) {
            break;
        }
        current = current->next;
        i++;
    }

    current->key = sds_replace(current->key, key);
    current->value_i = value_i;
    if (value_p != NULL) {
        current->value_p = sds_replace(current->value_p, value_p);
    }
    else if (current->value_p != NULL) {
        sdsclear(current->value_p);
    }
    if (current->user_data != NULL) {
        free(current->user_data);
    }
    current->user_data = user_data;
    return true;
}

bool list_push(struct t_list *l, const char *key, long value_i, const char *value_p, void *user_data) {
    struct t_list_node *n = malloc_assert(sizeof(struct t_list_node));
    n->key = sdsnew(key);
    n->value_i = value_i;
    if (value_p != NULL) {
        n->value_p = sdsnew(value_p);
    }
    else {
        n->value_p = sdsempty();
    }
    n->user_data = user_data;
    n->next = NULL;

    if (l->head == NULL) {
        l->head = n;
    }
    else if (l->tail != NULL) {
        l->tail->next = n;
    }
    else {
        FREE_SDS(n->value_p);
        FREE_SDS(n->key);
        free(n);
        return false;
    }

    l->tail = n;
    l->length++;
    return true;
}

bool list_push_len(struct t_list *l, const char *key, size_t key_len, long value_i, const char *value_p, size_t value_len, void *user_data) {
    struct t_list_node *n = malloc_assert(sizeof(struct t_list_node));
    n->key = sdsnewlen(key, key_len);
    n->value_i = value_i;
    if (value_p != NULL) {
        n->value_p = sdsnewlen(value_p, value_len);
    }
    else {
        n->value_p = sdsempty();
    }
    n->user_data = user_data;
    n->next = NULL;

    if (l->head == NULL) {
        l->head = n;
    }
    else if (l->tail != NULL) {
        l->tail->next = n;
    }
    else {
        FREE_SDS(n->value_p);
        FREE_SDS(n->key);
        free(n);
        return false;
    }

    l->tail = n;
    l->length++;
    return true;
}

bool list_insert(struct t_list *l, const char *key, long value_i, const char *value_p, void *user_data) {
    struct t_list_node *n = malloc_assert(sizeof(struct t_list_node));
    n->key = sdsnew(key);
    n->value_i = value_i;
    if (value_p != NULL) {
        n->value_p = sdsnew(value_p);
    }
    else {
        n->value_p = sdsempty();
    }
    n->user_data = user_data;
    n->next = l->head;

    l->head = n;
    l->length++;
    return true;
}

bool list_insert_sorted_by_key(struct t_list *l, const char *key, long value_i, const char *value_p, void *user_data, enum list_sort_direction direction) {
    struct t_list_node *n = malloc_assert(sizeof(struct t_list_node));
    n->key = sdsnew(key);
    n->value_i = value_i;
    if (value_p != NULL) {
        n->value_p = sdsnew(value_p);
    }
    else {
        n->value_p = sdsempty();
    }
    n->user_data = user_data;
    n->next = NULL;
    //empty list
    if (l->head == NULL) {
        l->head = n;
        l->tail = n;
        l->length++;
        return true;
    }
    //find correct position to insert
    struct t_list_node *current = NULL;
    struct t_list_node *previous = NULL;
    for (current = l->head; current != NULL; previous = current, current = current->next) {
        if (direction == LIST_SORT_ASC && strcmp(n->key, current->key) < 0) {
            break;
        }
        if (direction == LIST_SORT_DESC && strcmp(n->key, current->key) > 0) {
            break;
        }
    }
    //insert node
    if (previous != NULL) {
        previous->next = n;
    }
    else {
        l->head = n;
    }
    n->next = current;
    //fix tail
    if (l->tail == previous && previous != NULL) {
        l->tail = previous->next;
    }
    l->length++;

    return true;
}

bool list_insert_sorted_by_value_i(struct t_list *l, const char *key, long value_i, const char *value_p, void *user_data, enum list_sort_direction direction) {
    struct t_list_node *n = malloc_assert(sizeof(struct t_list_node));
    n->key = sdsnew(key);
    n->value_i = value_i;
    if (value_p != NULL) {
        n->value_p = sdsnew(value_p);
    }
    else {
        n->value_p = sdsempty();
    }
    n->user_data = user_data;
    n->next = NULL;
    //empty list
    if (l->head == NULL) {
        l->head = n;
        l->tail = n;
        l->length++;
        return true;
    }
    //find correct position to insert
    struct t_list_node *current = NULL;
    struct t_list_node *previous = NULL;
    for (current = l->head; current != NULL; previous = current, current = current->next) {
        if (direction == LIST_SORT_ASC && n->value_i < current->value_i) {
            break;
        }
        if (direction == LIST_SORT_DESC && n->value_i > current->value_i) {
            break;
        }
    }
    //insert node
    if (previous != NULL) {
        previous->next = n;
    }
    else {
        l->head = n;
    }
    n->next = current;
    //fix tail
    if (l->tail == previous && previous != NULL) {
        l->tail = previous->next;
    }
    l->length++;

    return true;
}

struct t_list_node *list_shift_first(struct t_list *l) {
    if (l->head == NULL) {
        return NULL;
    }

    struct t_list_node *extracted = l->head;
    l->head = l->head->next;
    if (l->tail == extracted) {
        l->tail = NULL;
    }

    extracted->next = NULL;
    return extracted;
}

void list_node_free(struct t_list_node *n) {
    list_node_free_user_data(n, NULL);
}

void list_node_free_user_data(struct t_list_node *n, user_data_callback free_cb) {
    FREE_SDS(n->key);
    FREE_SDS(n->value_p);
    if (n->user_data != NULL && free_cb != NULL) {
        free_cb(n);
    }
    else if (n->user_data != NULL) {
        free(n->user_data);
    }
    free(n);
}

bool list_shift(struct t_list *l, long idx) {
    struct t_list_node *extracted = list_node_extract(l, idx);
    if (extracted == NULL) {
        return false;
    }
    FREE_SDS(extracted->key);
    FREE_SDS(extracted->value_p);
    if (extracted->user_data != NULL) {
        free(extracted->user_data);
    }
    free(extracted);
    return true;
}

static struct t_list_node *list_node_extract(struct t_list *l, long idx) {
    if (l->head == NULL || idx >= l->length) {
        return NULL;
    }

    struct t_list_node *current = NULL;
    struct t_list_node *previous = NULL;
    long i = 0;
    for (current = l->head; current != NULL; previous = current, current = current->next) {
        if (i == idx) {
            if (previous == NULL) {
                //Fix head
                l->head = current->next;
            }
            else {
                //Fix previous nodes next to skip over the removed node
                previous->next = current->next;
            }
            //Fix tail
            if (l->tail == current) {
                l->tail = previous;
            }
            l->length--;
            break;
        }
        i++;
    }
    //null out this node's next value since it's not part of a list anymore
    if (current != NULL) {
        current->next = NULL;
    }
    return current;
}
