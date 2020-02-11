/*
 SPDX-License-Identifier: GPL-2.0-or-later
 myMPD (c) 2018-2020 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdio.h>
#include <stdbool.h>
#include <assert.h>

#include "../dist/src/sds/sds.h"
#include "sds_extras.h"
#include "list.h"

//private definitions
static struct list_node *list_node_extract(struct list *l, unsigned idx);

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
    unsigned int pos;
    int n = 0;

    if (l->length < 2) {
        return false;
    }

    struct list_node *current = l->head;
    while (current != NULL) {
        pos = rand() % l->length;
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
  
    if (l->head == NULL) 
        return false;
  
    do { 
        swapped = 0; 
        ptr1 = l->head;
  
        while (ptr1->next != lptr)  { 
            if (order == true && ptr1->value_i > ptr1->next->value_i) {  
                list_swap_item(ptr1, ptr1->next); 
                swapped = 1; 
            } 
            else if (order == false && ptr1->value_i < ptr1->next->value_i) {  
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
  
    if (l->head == NULL) 
        return false;
  
    do { 
        swapped = 0; 
        ptr1 = l->head;
  
        while (ptr1->next != lptr)  { 
            if (order == true && strcmp(ptr1->value_p, ptr1->next->value_p) > 0) {  
                list_swap_item(ptr1, ptr1->next); 
                swapped = 1; 
            } 
            else if (order == false && strcmp(ptr1->value_p, ptr1->next->value_p) < 0) {  
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
  
    if (l->head == NULL) 
        return false;
  
    do { 
        swapped = 0; 
        ptr1 = l->head;
  
        while (ptr1->next != lptr)  { 
            if (order == true && strcmp(ptr1->key, ptr1->next->key) > 0) {  
                list_swap_item(ptr1, ptr1->next); 
                swapped = 1; 
            } 
            else if (order == false && strcmp(ptr1->key, ptr1->next->key) < 0) {  
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

bool list_replace(struct list *l, int pos, const char *key, long value_i, const char *value_p, void *user_data) {
    if (pos >= l->length) {
        return false;
    }
    int i = 0;
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

bool list_shift(struct list *l, unsigned idx) {
    struct list_node * extracted = list_node_extract(l, idx);
    if (extracted == NULL) {
        return -1;
    }
    sdsfree(extracted->key);
    sdsfree(extracted->value_p);
    if (extracted->user_data != NULL) {
        free(extracted->user_data);
    }    
    free(extracted);
    return 0;
}

bool list_free(struct list *l) {
    struct list_node *current = l->head;
    struct list_node *tmp = NULL;
    while (current != NULL) {
        sdsfree(current->key);
        sdsfree(current->value_p);
        if (current->user_data != NULL) {
            free(current->user_data);
        }
        tmp = current;
        current = current->next;
        free(tmp);
    }
    list_init(l);
    return 0;
}

//private functions

static struct list_node *list_node_extract(struct list *l, unsigned idx) {
    if (l->head == NULL) { 
        return NULL; 
    }
    struct list_node *current = l->head, **previous = &l->head;
    for (; idx > 0; idx--) {
        if (current->next == NULL) {
            return NULL;
        }
        previous = &current->next;
        current = current->next;
    }
    //set tail to the previous node, if it is removed
    if (l->tail == current) {
        l->tail = *previous;
    }
    //set the previous node's 'next' value to the current nodes next value
    *previous = current->next;
    //null out this node's next value since it's not part of a list anymore
    current->next = NULL;
    l->length--;
    return current;
}