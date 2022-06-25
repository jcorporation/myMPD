/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2022 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include "mympd_config_defs.h"
#include "list.h"

#include "../../dist/utf8/utf8.h"
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

//Clears the list and frees all nodes and there values
void list_clear_user_data(struct t_list *l, user_data_callback free_cb) {
    struct t_list_node *current = l->head;
    struct t_list_node *tmp = NULL;
    while (current != NULL) {
        FREE_SDS(current->key);
        FREE_SDS(current->value_p);
        if (current->user_data != NULL &&
            free_cb != NULL)
        {
            //callback to free user_data
            free_cb(current);
        }
        else if (free_cb == NULL) {
            FREE_PTR(current->user_data);
        }
        tmp = current;
        current = current->next;
        FREE_PTR(tmp);
    }
    list_init(l);
}

//callback function to not free user_data
void list_free_cb_ignore_user_data(struct t_list_node *current) {
    //simply do nothing
    (void)current;
}

//callback function to free user_data of type sds
void list_free_cb_sds_user_data(struct t_list_node *current) {
    FREE_SDS(current->user_data);
}

//callback function to free user_data of type t_list
void list_free_cb_t_list_user_data(struct t_list_node *current) {
    list_clear((struct t_list *)current->user_data);
}

//gets a list node by key
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

//gets the list node at pos
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

//moves a node from one to another position
bool list_move_item_pos(struct t_list *l, long from, long to) {
    if (from >= l->length || to >= l->length) {
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

//swaps two list nodes by pos
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

//swaps two list nodes
bool list_swap_item(struct t_list_node *n1, struct t_list_node *n2) {
    if (n1 == n2) {
        return false;
    }

    if (n1 == NULL || n2 == NULL) {
        return false;
    }

    sds key = n2->key;
    long long value_i = n2->value_i;
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

//shuffles the list
bool list_shuffle(struct t_list *l) {
    if (l->length < 2) {
        return false;
    }
    int n = 0;
    struct t_list_node *current = l->head;
    while (current != NULL) {
        long pos = randrange(0, l->length - 1);
        list_swap_item(current, list_node_at(l, pos));
        n++;
        current = current->next;
    }
    return true;
}

//appends a node at the end of the list
bool list_push(struct t_list *l, const char *key, long long value_i,
        const char *value_p, void *user_data)
{
    size_t value_len = value_p == NULL ? 0 : strlen(value_p);
    return list_push_len(l, key, strlen(key), value_i, value_p, value_len, user_data);
}

//appends a node at the end of the list
//key and value_p len must be specified
bool list_push_len(struct t_list *l, const char *key, size_t key_len, long long value_i,
        const char *value_p, size_t value_len, void *user_data)
{
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
        FREE_PTR(n);
        return false;
    }

    l->tail = n;
    l->length++;
    return true;
}

//inserts a node at the beginning of the list
bool list_insert(struct t_list *l, const char *key, long long value_i,
        const char *value_p, void *user_data)
{
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

//replaces a list node at pos
//ignores the old user_data
bool list_replace(struct t_list *l, long pos, const char *key, long long value_i,
        const char *value_p, void *user_data)
{
    return list_replace_user_data(l, pos, key, value_i, value_p,
        user_data, list_free_cb_ignore_user_data);
}

//replaces a list node at pos
//frees the old user_data
bool list_replace_user_data(struct t_list *l, long pos, const char *key, long long value_i,
        const char *value_p, void *user_data, user_data_callback free_cb)
{
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
    if (current->user_data != NULL &&
        free_cb != NULL)
    {
        //callback to free old user_data
        free_cb(current);
    }
    else if (current->user_data != NULL) {
        FREE_PTR(current->user_data);
    }
    current->user_data = user_data;
    return true;
}

//removes the first node from the list and returns it
struct t_list_node *list_shift_first(struct t_list *l) {
    if (l->head == NULL) {
        return NULL;
    }

    struct t_list_node *extracted = l->head;
    l->head = l->head->next;
    if (l->tail == extracted) {
        l->tail = NULL;
    }
    l->length--;
    extracted->next = NULL;
    return extracted;
}

//frees a list node
//ignores user_data
void list_node_free(struct t_list_node *n) {
    list_node_free_user_data(n, list_free_cb_ignore_user_data);
}

//frees a list node and its user_data
void list_node_free_user_data(struct t_list_node *n, user_data_callback free_cb) {
    FREE_SDS(n->key);
    FREE_SDS(n->value_p);
    if (n->user_data != NULL &&
        free_cb != NULL)
    {
        //callback to free user_data
        free_cb(n);
    }
    else if (n->user_data != NULL) {
        FREE_PTR(n->user_data);
    }
    FREE_PTR(n);
}

//removes the node at idx from the list and frees it
//ignores user_data
bool list_remove_node(struct t_list *l, long idx) {
    return list_remove_node_user_data(l, idx, list_free_cb_ignore_user_data);
}

//removes the node at idx from the list and frees it and frees user_data
bool list_remove_node_user_data(struct t_list *l, long idx, user_data_callback free_cb) {
    struct t_list_node *extracted = list_node_extract(l, idx);
    if (extracted == NULL) {
        return false;
    }
    list_node_free_user_data(extracted, free_cb);
    return true;
}

//removes the node at idx from the list and returns it
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
