/*
 SPDX-License-Identifier: GPL-2.0-or-later
 myMPD (c) 2018-2021 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include "mympd_config_defs.h"
#include "list.h"

#include "random.h"
#include "sds_extras.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

//private definitions
static struct list_node *list_node_extract(struct list *l, unsigned idx);
static bool _list_free(struct list *l, bool free_user_data);

//public functions
bool list_init(struct list *l) {
    l->length = 0;
    l->head = NULL;
    l->tail = NULL;
    return true;
}

long list_get_value_i(const struct list *l, const char *key) {
    long value_i = -1;
    struct list_node *current = l->head;
    while (current != NULL) {
        if (strcmp(current->key, key) == 0) {
            value_i = current->value_i;
            break;
        }
        current = current->next;
    }
    return value_i;
}

sds list_get_value_p(const struct list *l, const char *key) {
    sds value_p = NULL;
    struct list_node *current = l->head;
    while (current != NULL) {
        if (strcmp(current->key, key) == 0) {
            value_p = current->value_p;
            break;
        }
        current = current->next;
    }
    return value_p;
}

void *list_get_user_data(const struct list *l, const char *key) {
    void *user_data = NULL;
    struct list_node *current = l->head;
    while (current != NULL) {
        if (strcmp(current->key, key) == 0) {
            user_data = current->user_data;
            break;
        }
        current = current->next;
    }
    return user_data;
}

struct list_node *list_get_node(const struct list *l, const char *key) {
    struct list_node *current = l->head;
    while (current != NULL) {
        if (strcmp(current->key, key) == 0) {
            break;
        }
        current = current->next;
    }
    return current;
}

struct list_node *list_node_at(const struct list *l, unsigned index) {
    /* if there's no data in the list, fail */
    if (l->head == NULL) { 
        return NULL; 
    }
    struct list_node * current = l->head;
    for (; index > 0; index--) {
        if (current->next == NULL) { 
            return NULL;
        }
        current = current->next;
    }
    return current;
}

bool list_move_item_pos(struct list *l, unsigned from, unsigned to) {
    if (from > l->length || to > l->length) {
        return false;
    }
    if (from == to) {
        return true;
    }

    //extract node at from position;
    struct list_node *node = list_node_extract(l, from);
    if (node == NULL) {
        return false;
    }

    struct list_node *current = l->head;
    struct list_node **previous = &l->head;
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

bool list_swap_item_pos(struct list *l, unsigned index1, unsigned index2) {
   if (l->length < 2) {
        return false;
    }
    struct list_node *node1 = list_node_at(l, index1);
    struct list_node *node2 = list_node_at(l, index2);
    if (node1 == NULL || node2 == NULL) {
        return false;
    }
    return list_swap_item(node1, node2);
}

bool list_swap_item(struct list_node *n1, struct list_node *n2) {
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

bool list_shuffle(struct list *l) {
    if (l->length < 2) {
        return false;
    }
    int n = 0;
    struct list_node *current = l->head;
    while (current != NULL) {
        unsigned int pos = randrange(0, l->length);
        list_swap_item(current, list_node_at(l, pos));
        n++;
        current = current->next;
    }
    return true;
}

bool list_sort_by_value_i(struct list *l, bool order) {
    int swapped; 
    struct list_node *ptr1; 
    struct list_node *lptr = NULL; 
  
    if (l->head == NULL) {
        return false;
    }
  
    do { 
        swapped = 0; 
        ptr1 = l->head;
  
        while (ptr1->next != lptr)  { 
            if ((order == true && ptr1->value_i > ptr1->next->value_i) ||
                (order == false && ptr1->value_i < ptr1->next->value_i))
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

bool list_sort_by_value_p(struct list *l, bool order) {
    int swapped; 
    struct list_node *ptr1; 
    struct list_node *lptr = NULL; 
  
    if (l->head == NULL) {
        return false;
    }
  
    do { 
        swapped = 0; 
        ptr1 = l->head;
  
        while (ptr1->next != lptr)  { 
            if ((order == true && strcmp(ptr1->value_p, ptr1->next->value_p) > 0) ||
                (order == false && strcmp(ptr1->value_p, ptr1->next->value_p) < 0))
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

bool list_sort_by_key(struct list *l, bool order) {
    int swapped; 
    struct list_node *ptr1; 
    struct list_node *lptr = NULL; 
  
    if (l->head == NULL) {
        return false;
    }
  
    do { 
        swapped = 0; 
        ptr1 = l->head;
  
        while (ptr1->next != lptr)  { 
            if ((order == true && strcmp(ptr1->key, ptr1->next->key) > 0) || 
                (order == false && strcmp(ptr1->key, ptr1->next->key) < 0))
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

bool list_replace(struct list *l, unsigned pos, const char *key, long value_i, const char *value_p, void *user_data) {
    if (pos >= l->length) {
        return false;
    }
    unsigned i = 0;
    struct list_node *current = l->head;
    while (current->next != NULL) {
        if (i == pos) {
            break;
        }
        current = current->next;
        i++;
    }
    
    current->key = sdsreplace(current->key, key);
    current->value_i = value_i;
    current->value_p = sdsreplace(current->value_p, value_p);
    if (value_p != NULL) {
        current->value_p = sdsreplace(current->value_p, value_p);
    }
    else {
        current->value_p = sdscrop(current->value_p);
    }
    if (current->user_data != NULL) {
        free(current->user_data);
    }
    current->user_data = user_data;
    return true;
}

bool list_push(struct list *l, const char *key, long value_i, const char *value_p, void *user_data) {
    struct list_node *n = malloc(sizeof(struct list_node));
    assert(n);
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
        sdsfree(n->value_p);
        sdsfree(n->key);
        free(n);
        return false;
    }

    l->tail = n;
    l->length++;
    return true;
}

bool list_push_len(struct list *l, const char *key, int key_len, long value_i, const char *value_p, int value_len, void *user_data) {
    struct list_node *n = malloc(sizeof(struct list_node));
    assert(n);
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
        sdsfree(n->value_p);
        sdsfree(n->key);
        free(n);
        return false;
    }

    l->tail = n;
    l->length++;
    return true;
}

bool list_insert(struct list *l, const char *key, long value_i, const char *value_p, void *user_data) {
    struct list_node *n = malloc(sizeof(struct list_node));
    assert(n);
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

bool list_insert_sorted_by_key(struct list *l, const char *key, long value_i, const char *value_p, void *user_data, bool order) {
    struct list_node *n = malloc(sizeof(struct list_node));
    assert(n);
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
    struct list_node *current = NULL;
    struct list_node *previous = NULL;
    for (current = l->head; current != NULL; previous = current, current = current->next) {    
        if (order == false && strcmp(n->key, current->key) < 0) {
            break;
        }
        if (order == true && strcmp(n->key, current->key) > 0) {
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

bool list_insert_sorted_by_value_i(struct list *l, const char *key, long value_i, const char *value_p, void *user_data, bool order) {
    struct list_node *n = malloc(sizeof(struct list_node));
    assert(n);
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
    struct list_node *current = NULL;
    struct list_node *previous = NULL;
    for (current = l->head; current != NULL; previous = current, current = current->next) {    
        if (order == false && n->value_i < current->value_i) {
            break;
        }
        if (order == true && n->value_i > current->value_i) {
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

struct list_node *list_shift_first(struct list *l) {
    if (l->head == NULL) {
        return NULL;
    }
    
    struct list_node *extracted = l->head;
    l->head = l->head->next;
    if (l->tail == extracted) {
        l->tail = NULL;
    }
    
    extracted->next = NULL;
    return extracted;
}

bool list_node_free(struct list_node *n) {
    sdsfree(n->key);
    sdsfree(n->value_p);
    if (n->user_data != NULL) {
        free(n->user_data);
    }
    free(n);
    return true;
}

bool list_node_free_keep_user_data(struct list_node *n) {
    sdsfree(n->key);
    sdsfree(n->value_p);
    free(n);
    return true;
}

bool list_shift(struct list *l, unsigned idx) {
    struct list_node *extracted = list_node_extract(l, idx);
    if (extracted == NULL) {
        return false;
    }
    sdsfree(extracted->key);
    sdsfree(extracted->value_p);
    if (extracted->user_data != NULL) {
        free(extracted->user_data);
    }
    free(extracted);
    return true;
}

bool list_free(struct list *l) {
    return _list_free(l, true);
}

bool list_free_keep_user_data(struct list *l) {
    return _list_free(l, false);
}

//private functions
static bool _list_free(struct list *l, bool free_user_data) {
    struct list_node *current = l->head;
    struct list_node *tmp = NULL;
    while (current != NULL) {
        sdsfree(current->key);
        sdsfree(current->value_p);
        if (free_user_data == true && current->user_data != NULL) {
            free(current->user_data);
        }
        tmp = current;
        current = current->next;
        free(tmp);
    }
    list_init(l);
    return true;
}

static struct list_node *list_node_extract(struct list *l, unsigned idx) {
    if (l->head == NULL || idx >= l->length) { 
        return NULL; 
    }

    struct list_node *current = NULL;
    struct list_node *previous = NULL;
    unsigned i = 0;
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
