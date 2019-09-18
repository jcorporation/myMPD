/* myMPD
   (c) 2018-2019 Juergen Mang <mail@jcgames.de>
   This project's homepage is: https://github.com/jcorporation/mympd
   
   This linked list implementation is based on: https://github.com/joshkunz/ashuffle
   
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; version 2 of the License.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License along
   with this program; if not, write to the Free Software Foundation, Inc.,
   Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdio.h>
#include <stdbool.h>
#include <assert.h>

#include "../dist/src/sds/sds.h"
#include "list.h"

int list_init(struct list *l) {
    l->length = 0;
    l->list = NULL;
    return 0;
}

int list_get_value(const struct list *l, const char *data) {
    int value = -1;
    struct node *current = l->list;
    while (current != NULL) {
        if (strcmp(current->data, data) == 0) {
            value = current->value;
            break;
        }
        current = current->next;
    }
    return value;
}

void *list_get_extra(const struct list *l, const char *data) {
    void *extra = NULL;
    struct node *current = l->list;
    while (current != NULL) {
        if (strcmp(current->data, data) == 0) {
            extra = current->extra;
            break;
        }
        current = current->next;
    }
    return extra;
}

struct node *list_get_node(const struct list *l, const char *data) {
    struct node *current = l->list;
    while (current != NULL) {
        if (strcmp(current->data, data) == 0) {
            break;
        }
        current = current->next;
    }
    return current;
}

struct node *list_node_at(const struct list *l, unsigned index) {
    /* if there's no data in the list, fail */
    if (l->list == NULL) { return NULL; }
    struct node * current = l->list;
    for (; index > 0; index--) {
        if (current->next == NULL) { return NULL; }
        current = current->next;
    }
    return current;
}

int list_swap_item(struct node *n1, struct node *n2) {
    if (n1 == n2)
        return 1;
        
    if (n1 == NULL || n2 == NULL)
        return 1;
        
    int value = n2->value;
    sds data = n2->data;
    void *extra = n2->extra;
    
    n2->value = n1->value;
    n2->data = n1->data;
    n2->extra = n1->extra;
    
    n1->value = value;
    n1->data = data;
    n1->extra = extra;
    
    return 0;
}

int list_shuffle(struct list *l) {
    int pos;
    int n = 0;

    if (l->length < 2)
        return 1;

    struct node *current = l->list;
    while (current != NULL) {
        pos = rand() / (RAND_MAX / (l->length - n + 1) + 1);
        list_swap_item(current, list_node_at(l, pos));
        n++;
        current = current->next;
    }
    return 0;
}

int list_sort_by_value(struct list *l, bool order) {
    int swapped; 
    struct node *ptr1; 
    struct node *lptr = NULL; 
  
    if (l->list == NULL) 
        return 1; 
  
    do { 
        swapped = 0; 
        ptr1 = l->list;
  
        while (ptr1->next != lptr)  { 
            if (order == true && ptr1->value > ptr1->next->value) {  
                list_swap_item(ptr1, ptr1->next); 
                swapped = 1; 
            } 
            else if (order == false && ptr1->value < ptr1->next->value) {  
                list_swap_item(ptr1, ptr1->next); 
                swapped = 1; 
            } 
            ptr1 = ptr1->next; 
        } 
        lptr = ptr1; 
    } 
    while (swapped);
    return 0; 
}

int list_replace(struct list *l, int pos, const char *data, int value, void *extra) {
    int i = 0;
    struct node *current = l->list;
    while (current->next != NULL) {
        if (i == pos)
            break;
        current = current->next;
        i++;
    }
    
    current->value = value;
    current->data = sdscat(sdsempty(), data);
    current->extra = extra;
    return 0;
}

int list_push(struct list *l, const char *data, int value, void *extra) {
    struct node *n = malloc(sizeof(struct node));
    assert(n);
    n->value = value;
    n->data = sdsnew(data);
    n->extra = extra;
    n->next = NULL;

    struct node **next = &l->list;
    while (*next != NULL) {
        next = &(*next)->next;
    }
    *next = n;
    l->length++;
    return 0;
}

int list_insert(struct list *l, const char *data, int value, void *extra) {
    struct node *n = malloc(sizeof(struct node));
    assert(n);
    n->value = value;
    n->data = sdsnew(data);
    n->extra = extra;
    n->next = l->list;
    
    l->list = n;
    l->length++;
    return 0;
}

struct node *list_node_extract(struct list *l, unsigned idx) {
    if (l->list == NULL) { return NULL; }
    struct node *current = l->list, **previous = &l->list;
    for (; idx > 0; idx--) {
        if (current->next == NULL) {
            return NULL;
        }
        previous = &current->next;
        current = current->next;
    }
    /* set the previous node's 'next' value to the current
     * nodes next value */
    *previous = current->next;
    /* null out this node's next value since it's not part of
     * a list anymore */
    current->next = NULL;
    l->length--;
    return current;
}

int list_shift(struct list *l, unsigned idx) {
    struct node * extracted = list_node_extract(l, idx);
    if (extracted == NULL) 
        return -1;
    sds_free(extracted->data);
    free(extracted->extra);
    free(extracted);
    return 0;
}

int list_free(struct list *l) {
    struct node *current = l->list, *tmp = NULL;
    while (current != NULL) {
        sds_free(current->data);
        if (current->extra != NULL) {
            free(current->extra);
            current->extra = NULL;
        }
        tmp = current;
        current = current->next;
        free(tmp);
    }
    list_init(l);
    return 0;
}
