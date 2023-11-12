/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2023 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include "compile_time.h"
#include "src/lib/list.h"

#include "dist/utf8/utf8.h"
#include "src/lib/filehandler.h"
#include "src/lib/log.h"
#include "src/lib/mem.h"
#include "src/lib/random.h"
#include "src/lib/sds_extras.h"

#include <string.h>

/**
 * Mallocs a new list and inits it.
 * @return allocated empty list
 */
struct t_list *list_new(void) {
    struct t_list *l = malloc_assert(sizeof(struct t_list));
    list_init(l);
    return l;
}

/**
 * Inits a already allocated list
 * @param l pointer to list
 */
void list_init(struct t_list *l) {
    l->length = 0;
    l->head = NULL;
    l->tail = NULL;
}

/**
 * Clears the list and frees all nodes, ignores user_data
 * @param l pointer to list
 */
void list_clear(struct t_list *l) {
    list_clear_user_data(l, list_free_cb_ignore_user_data);
}

/**
 * Clears the list, frees all nodes and the list itself, ignores user_data
 * @param l pointer to list
 * @return NULL
 */
void *list_free(struct t_list *l) {
    list_clear_user_data(l, list_free_cb_ignore_user_data);
    FREE_PTR(l);
    return NULL;
}

/**
 * Clears the list and frees all nodes and there values, calls a function to free user_data,
 * set free_cb to NULL, to free a generic pointer.
 * @param l pointer to list
 * @param free_cb
 */
void list_clear_user_data(struct t_list *l, user_data_callback free_cb) {
    struct t_list_node *current = l->head;
    struct t_list_node *tmp = NULL;
    while (current != NULL) {
        tmp = current;
        current = current->next;
        list_node_free_user_data(tmp, free_cb);
    }
    list_init(l);
}

/**
 * Clears the list and frees all nodes and there values, calls a function to free user_data,
 * set free_cb to NULL, to free a generic pointer.
 * @param l pointer to list
 * @param free_cb
 */
void *list_free_user_data(struct t_list *l, user_data_callback free_cb) {
    list_clear_user_data(l, free_cb);
    FREE_PTR(l);
    return NULL;
}

/**
 * Callback function to not free user_data.
 * @param current list node
 */
void list_free_cb_ignore_user_data(struct t_list_node *current) {
    //simply do nothing
    (void)current;
}

/**
 * Callback function to free user_data of type sds.
 * @param current list node
 */
void list_free_cb_sds_user_data(struct t_list_node *current) {
    FREE_SDS(current->user_data);
}

/**
 * Callback function to free user_data of generic pointer.
 * @param current list node
 */
void list_free_cb_ptr_user_data(struct t_list_node *current) {
    FREE_PTR(current->user_data);
}

/**
 * Gets a list node index by key.
 * @param l list
 * @param key key to get
 * @return int index of the key, -1 if not found
 */
int list_get_node_idx(const struct t_list *l, const char *key) {
    struct t_list_node *current = l->head;
    int i = 0;
    while (current != NULL) {
        if (strcmp(current->key, key) == 0) {
            return i;
        }
        current = current->next;
        i++;
    }
    return -1;
}

/**
 * Gets a list node by key.
 * @param l list
 * @param key key to get
 * @return pointer to list node
 */
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

/**
 * Gets the list node at idx and its previous node.
 * @param l list
 * @param idx node index to get
 * @param previous pointer to previous node
 * @return pointer to list node
 */
struct t_list_node *list_node_prev_at(const struct t_list *l, long idx, struct t_list_node **previous) {
    //if there's no data in the list, fail
    if (l->head == NULL) {
        return NULL;
    }
    if (idx >= l->length) {
        return NULL;
    }
    struct t_list_node *current = l->head;
    *previous = NULL;
    for (; idx > 0; idx--) {
        *previous = current;
        current = current->next;
    }
    return current;
}

/**
 * Gets the list node at idx
 * @param l list
 * @param idx node index to get
 * @return pointer to list node
 */
struct t_list_node *list_node_at(const struct t_list *l, long idx) {
    struct t_list_node *previous = NULL;
    return list_node_prev_at(l, idx, &previous);
}

/**
 * Moves a node in the list to another position.
 * @param l list
 * @param from from pos
 * @param to to pos
 * @return true on success, else false
 */
bool list_move_item_pos(struct t_list *l, long from, long to) {
    if (from >= l->length ||
        to >= l->length ||
        from < 0 ||
        to < 0)
    {
        return false;
    }
    if (from == to) {
        return true;
    }

    bool after = from < to ? true : false;

    //get new position
    struct t_list_node *previous = NULL;
    struct t_list_node *current = list_node_prev_at(l, to, &previous);

    //extract node from position;
    struct t_list_node *node = list_node_extract(l, from);
    if (node == NULL) {
        return false;
    }

    if (after == true) {
        node->next = current->next;
        current->next = node;
        if (l->tail == current) {
            l->tail = node;
        }
    }
    else {
        node->next = current;
        if (l->head == current) {
            l->head = node;
        }
        else {
            previous->next = node;
        }
    }

    l->length++;

    return true;
}

/**
 * Swaps two list nodes values.
 * @param n1 first node
 * @param n2 second node
 * @return true on success, else false
 */
bool list_swap_item(struct t_list_node *n1, struct t_list_node *n2) {
    if (n1 == n2 ||
        n1 == NULL ||
        n2 == NULL)
    {
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

/**
 * Shuffles the list.
 * @param l list
 * @return true on success, else false
 */
bool list_shuffle(struct t_list *l) {
    if (l->length < 2) {
        return false;
    }
    struct t_list_node *current = l->head;
    while (current != NULL) {
        long pos = randrange(0, l->length - 1);
        list_swap_item(current, list_node_at(l, pos));
        current = current->next;
    }
    return true;
}

/**
 * Creates and appends a node to the end of the list.
 * @param l list
 * @param key key value
 * @param value_i long long value
 * @param value_p sds value
 * @param user_data pointer to user_data
 * @return true on success, else false
 */
bool list_push(struct t_list *l, const char *key, long long value_i,
        const char *value_p, void *user_data)
{
    size_t value_len = value_p == NULL ? 0 : strlen(value_p);
    return list_push_len(l, key, strlen(key), value_i, value_p, value_len, user_data);
}

/**
 * Creates and appends a node to the end of the list.
 * key and value_p len must be specified.
 * @param l list
 * @param key key value
 * @param key_len key length
 * @param value_i long long value
 * @param value_p sds value
 * @param value_len value_p length
 * @param user_data pointer to user_data
 * @return true on success, else false
 */
bool list_push_len(struct t_list *l, const char *key, size_t key_len, long long value_i,
        const char *value_p, size_t value_len, void *user_data)
{
    //create new node
    struct t_list_node *n = malloc_assert(sizeof(struct t_list_node));
    n->key = sdsnewlen(key, key_len);
    n->value_i = value_i;
    n->value_p = value_p != NULL ? sdsnewlen(value_p, value_len) : NULL;
    n->user_data = user_data;
    n->next = NULL;

    if (l->head == NULL) {
        //first entry in the list
        l->head = n;
    }
    else {
        //append to the list
        l->tail->next = n;
    }
    //set tail and increase length
    l->tail = n;
    l->length++;
    return true;
}

/**
 * Creates and inserts a node at the beginning of the list.
 * @param l list
 * @param key key value
 * @param value_i long long value
 * @param value_p sds value
 * @param user_data pointer to user_data
 * @return true on success, else false
 */
bool list_insert(struct t_list *l, const char *key, long long value_i,
        const char *value_p, void *user_data)
{
    //create new node
    struct t_list_node *n = malloc_assert(sizeof(struct t_list_node));
    n->key = sdsnew(key);
    n->value_i = value_i;
    n->value_p = value_p != NULL ? sdsnew(value_p) : NULL;
    n->user_data = user_data;

    //switch head pointer
    n->next = l->head;
    l->head = n;
    if (l->tail == NULL) {
        //set tail if this is the first node in the list
        l->tail = n;
    }
    l->length++;
    return true;
}

/**
 * Replaces a list nodes values at pos.
 * Ignores the old user_data pointer.
 * @param l list
 * @param idx index of node to change
 * @param key new key value
 * @param value_i new long long value
 * @param value_p new sds value
 * @param user_data new user_data pointer
 * @return true on success, else false
 */
bool list_replace(struct t_list *l, long idx, const char *key, long long value_i,
        const char *value_p, void *user_data)
{
    size_t value_len = value_p == NULL
        ? 0
        : strlen(value_p);
    return list_replace_len_user_data(l, idx, key, strlen(key), value_i,
        value_p, value_len, user_data, list_free_cb_ignore_user_data);
}

/**
 * Replaces a list nodes values at pos.
 * Ignores the old user_data pointer.
 * @param l list
 * @param idx index of node to change
 * @param key new key value
 * @param key_len new key value length
 * @param value_i new long long value
 * @param value_p new sds value
 * @param value_len new sds value len
 * @param user_data new user_data pointer
 * @return true on success, else false
 */
bool list_replace_len(struct t_list *l, long idx, const char *key, size_t key_len, long long value_i,
        const char *value_p, size_t value_len, void *user_data)
{
    return list_replace_len_user_data(l, idx, key, key_len, value_i,
        value_p, value_len, user_data, list_free_cb_ignore_user_data);
}

/**
 * Replaces a list nodes values at pos.
 * Frees the old user_data pointer.
 * @param l list
 * @param idx index of node to change
 * @param key new key value
 * @param value_i new long long value
 * @param value_p new sds value
 * @param user_data new user_data pointer
 * @param free_cb callback function to free old user_data pointer
 * @return true on success, else false
 */
bool list_replace_user_data(struct t_list *l, long idx, const char *key, long long value_i,
        const char *value_p, void *user_data, user_data_callback free_cb)
{
    return list_replace_len_user_data(l, idx, key, strlen(key), value_i,
        value_p, strlen(value_p), user_data, free_cb);
}

/**
 * Replaces a list nodes values at pos.
 * Frees the old user_data pointer.
 * @param l list
 * @param idx index of node to change
 * @param key new key value
 * @param key_len new key value length
 * @param value_i new long long value
 * @param value_p new sds value
 * @param value_len new sds value len
 * @param user_data new user_data pointer
 * @param free_cb callback function to free old user_data pointer
 * @return true on success, else false
 */
bool list_replace_len_user_data(struct t_list *l, long idx, const char *key, size_t key_len, long long value_i,
        const char *value_p, size_t value_len, void *user_data, user_data_callback free_cb)
{
    if (idx >= l->length) {
        return false;
    }
    struct t_list_node *current = list_node_at(l, idx);

    current->key = sds_replacelen(current->key, key, key_len);
    current->value_i = value_i;
    if (value_p != NULL) {
        current->value_p = sds_replacelen(current->value_p, value_p, value_len);
    }
    else if (current->value_p != NULL) {
        FREE_SDS(current->value_p);
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

/**
 * Frees a list node, ignoring its user_data pointer.
 * @param n node to free
 */
void *list_node_free(struct t_list_node *n) {
    list_node_free_user_data(n, list_free_cb_ignore_user_data);
    return NULL;
}

/**
 * Frees a list node and its user_data pointer
 * @param n node to free
 * @param free_cb callback function to free user_data pointer
 */
void *list_node_free_user_data(struct t_list_node *n, user_data_callback free_cb) {
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
    return NULL;
}

/**
 * Removes the node at idx from the list and frees it.
 * Ignores the user_data pointer.
 * @param l list
 * @param idx node index to free
 * @return true on success, else false
 */
bool list_remove_node(struct t_list *l, long idx) {
    return list_remove_node_user_data(l, idx, list_free_cb_ignore_user_data);
}

/**
 * Removes the node at idx from the list and frees it and frees the user_data pointer
 * @param l list
 * @param idx node index to free
 * @param free_cb callback function to free user_data pointer
 * @return true on success, else false
 */
bool list_remove_node_user_data(struct t_list *l, long idx, user_data_callback free_cb) {
    struct t_list_node *extracted = list_node_extract(l, idx);
    if (extracted == NULL) {
        return false;
    }
    list_node_free_user_data(extracted, free_cb);
    return true;
}

/**
 * Removes the first node from the list and returns it.
 * This is only a shortcut for list_node_extract.
 * @param l list
 * @return pointer to list node
 */
struct t_list_node *list_shift_first(struct t_list *l) {
    return list_node_extract(l, 0);
}

/**
 * Removes the node at idx from the list and returns it.
 * @param l list
 * @param idx node index to remove
 * @return pointer to list node
 */
struct t_list_node *list_node_extract(struct t_list *l, long idx) {
    if (l->head == NULL ||
        idx >= l->length) {
        return NULL;
    }

    //get the node at idx and the previous node
    struct t_list_node *previous = NULL;
    struct t_list_node *current = list_node_prev_at(l, idx, &previous);

    if (current == NULL) {
        return NULL;
    }

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

    //null out this node's next value since it's not part of a list anymore
    current->next = NULL;

    //return the node
    return current;
}

/**
 * Saves the list to disk
 * @param filepath filepath to write the list
 * @param l list to save
 * @param node_to_line_cb callback function to write a list node
 * @return true on success, else false
 */
bool list_write_to_disk(sds filepath, struct t_list *l, list_node_to_line_callback node_to_line_cb) {
    sds tmp_file = sdscatfmt(sdsempty(), "%S.XXXXXX", filepath);
    FILE *fp = open_tmp_file(tmp_file);
    if (fp == NULL) {
        FREE_SDS(tmp_file);
        return false;
    }
    struct t_list_node *current = l->head;
    sds buffer = sdsempty();
    bool write_rc = true;
    while (current != NULL) {
        buffer = node_to_line_cb(buffer, current);
        if (fputs(buffer, fp) == EOF) {
            MYMPD_LOG_ERROR(NULL, "Could not write data to file");
            write_rc = false;
            break;
        }
        sdsclear(buffer);
        current = current->next;
    }
    FREE_SDS(buffer);
    bool rc = rename_tmp_file(fp, tmp_file, write_rc);
    FREE_SDS(tmp_file);
    return rc;
}

/**
 * Crops the list to the defined length
 * @param l pointer to list to crop
 * @param length max length
 * @param free_cb callback function to free user_data pointer
 */
void list_crop(struct t_list *l, long length, user_data_callback free_cb) {
    if (l->length <= length) {
        return;
    }
    long idx = length - 1;
    if (idx < 0) {
        list_clear(l);
        return;
    }
    struct t_list_node *last = list_node_at(l, idx);
    struct t_list_node *current = last->next;
    while (current != NULL) {
        struct t_list_node *next = current->next;
        list_node_free_user_data(current, free_cb);
        current = next;
    }
    l->length = length;
    last->next = NULL;
    l->tail = last;
}

/**
 * Internal compare function to sort by value_i
 * @param current current list node
 * @param next next list node
 * @param direction sort direction
 * @return true if current is greater than next
 */
static bool list_sort_cmp_value_i(struct t_list_node *current, struct t_list_node *next, enum list_sort_direction direction) {
    if ((direction == LIST_SORT_ASC && current->value_i > next->value_i) ||
        (direction == LIST_SORT_DESC && current->value_i < next->value_i))
    {
        return true;
    }
    return false;
}

/**
 * Internal compare function to sort by value_p
 * @param current current list node
 * @param next next list node
 * @param direction sort direction
 * @return true if current is greater than next
 */
static bool list_sort_cmp_value_p(struct t_list_node *current, struct t_list_node *next, enum list_sort_direction direction) {
    int result = utf8casecmp(current->value_p, next->value_p);
    if ((direction == LIST_SORT_ASC && result > 0) ||
        (direction == LIST_SORT_DESC && result < 0))
    {
        return true;
    }
    return false;
}

/**
 * Internal compare function to sort by key
 * @param current current list node
 * @param next next list node
 * @param direction sort direction
 * @return true if current is greater than next
 */
static bool list_sort_cmp_key(struct t_list_node *current, struct t_list_node *next, enum list_sort_direction direction) {
    int result = utf8casecmp(current->key, next->key);
    if ((direction == LIST_SORT_ASC && result > 0) ||
        (direction == LIST_SORT_DESC && result < 0))
    {
        return true;
    }
    return false;
}

/**
 * The list sorting function.
 * Note that the sort is not very efficient, use it only for short lists.
 * @param l pointer to list to sort
 * @param direction sort direction
 * @param sort_cb compare function
 * @return true on success, else false
 */
bool list_sort_by_callback(struct t_list *l, enum list_sort_direction direction, list_sort_callback sort_cb) {
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
            if (sort_cb(ptr1, ptr1->next, direction) == true) {
                list_swap_item(ptr1, ptr1->next);
                swapped = 1;
            }
            ptr1 = ptr1->next;
        }
        lptr = ptr1;
    } while (swapped);
    return true;
}

/**
 * Sorts the list by value_i
 * @param l pointer to list to sort
 * @param direction sort direction
 * @return true on success, else false
 */
bool list_sort_by_value_i(struct t_list *l, enum list_sort_direction direction) {
    return list_sort_by_callback(l, direction, list_sort_cmp_value_i);
}

/**
 * Sorts the list by value_p
 * @param l pointer to list to sort
 * @param direction sort direction
 * @return true on success, else false
 */
bool list_sort_by_value_p(struct t_list *l, enum list_sort_direction direction) {
    return list_sort_by_callback(l, direction, list_sort_cmp_value_p);
}

/**
 * Sorts the list by key
 * @param l pointer to list to sort
 * @param direction sort direction
 * @return true on success, else false
 */
bool list_sort_by_key(struct t_list *l, enum list_sort_direction direction) {
    return list_sort_by_callback(l, direction, list_sort_cmp_key);
}
