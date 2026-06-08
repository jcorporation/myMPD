/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2026 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

/*! \file
 * \brief Linked list implementation
 */

#include "src/lib/list/list.h"

#include "src/lib/filehandler.h"
#include "src/lib/log.h"
#include "src/lib/mem.h"
#include "src/lib/sds/sds_extras.h"

#include <limits.h>
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
 * Duplicates a list.
 * Leaves user_data pointer in place.
 * @param l list to duplicate
 * @return duplicated list or NULL on error
 */
struct t_list *list_dup(struct t_list *l) {
    struct t_list *new = list_new();
    if (list_append(new, l) == false) {
        list_free(new);
        return NULL;
    }
    return new;
}

/**
 * Appends a list to another list
 * Leaves user_data pointer in place.
 * @param dst list to that was append
 * @param src list that was append
 * @return duplicated list or NULL on error
 */
bool list_append(struct t_list *dst, struct t_list *src) {
    struct t_list_node *current = src->head;
    while (current != NULL) {
        struct t_list_node *n = malloc_assert(sizeof(struct t_list_node));
        n->key = sdsdup(current->key);
        n->value_i = current->value_i;
        n->value_p = current->value_p != NULL
            ? sdsdup(current->value_p)
            : NULL;
        n->user_data = current->user_data;
        n->next = NULL;

        if (dst->head == NULL) {
            n->prev = NULL;
            dst->head = n;
        } else {
            n->prev = dst->tail;
            dst->tail->next = n;
        }
        dst->tail = n;
        dst->length++;

        current = current->next;
    }
    return true;
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
 */
void list_free(struct t_list *l) {
    if (l == NULL) {
        return;
    }
    list_clear_user_data(l, list_free_cb_ignore_user_data);
    FREE_PTR(l);
}

/**
 * Clears the list, frees all nodes and the list itself, ignores user_data
 * @param l pointer to list
 */
void list_free_void(void *l) {
    list_free((struct t_list *)l);
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
 * @param free_cb Callback to free the user data
 */
void list_free_user_data(struct t_list *l, user_data_callback free_cb) {
    list_clear_user_data(l, free_cb);
    FREE_PTR(l);
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
 * @return int index of the key, UINT_MAX if not found
 */
unsigned list_get_node_idx(const struct t_list *l, const char *key) {
    if (l->head == NULL) {
        return UINT_MAX;
    }

    size_t key_len = strlen(key);
    struct t_list_node *forward = l->head;
    struct t_list_node *backward = l->tail;

    // Search from both ends simultaneously, meeting in the middle
    for (unsigned i = 0, j = l->length / 2; i <= j; i++) {
        // Check forward pointer
        if (sdslen(forward->key) == key_len &&
            strncmp(forward->key, key, key_len) == 0)
        {
            return i;
        }

        // Check backward pointer (avoid duplicate check if single element)
        unsigned back_idx = l->length - 1 - i;
        if (i != back_idx &&
            sdslen(backward->key) == key_len &&
            strncmp(backward->key, key, key_len) == 0)
        {
            return back_idx;
        }

        // Move pointers for next iteration
        if (i < j) {
            forward = forward->next;
            backward = backward->prev;
        }
    }

    return UINT_MAX;
}

/**
 * Gets a list node by key.
 * @param l list
 * @param key key to get
 * @return pointer to list node
 */
struct t_list_node *list_get_node(const struct t_list *l, const char *key) {
    if (l->head == NULL) {
        return NULL;
    }

    size_t key_len = strlen(key);
    struct t_list_node *forward = l->head;
    struct t_list_node *backward = l->tail;

    // Search from both ends simultaneously, meeting in the middle
    for (unsigned i = 0, j = l->length / 2; i <= j; i++) {
        // Check forward pointer
        if (sdslen(forward->key) == key_len &&
            strncmp(forward->key, key, key_len) == 0)
        {
            return forward;
        }

        // Check backward pointer (avoid duplicate check if single element)
        if (i != l->length - 1 - i &&
            sdslen(backward->key) == key_len &&
            strncmp(backward->key, key, key_len) == 0)
        {
            return backward;
        }

        // Move pointers for next iteration
        if (i < j) {
            forward = forward->next;
            backward = backward->prev;
        }
    }

    return NULL;
}

/**
 * Gets the list node at idx and its previous node.
 * @param l list
 * @param idx node index to get
 * @return pointer to list node
 */
struct t_list_node *list_node_at(const struct t_list *l, unsigned idx) {
    if (l->head == NULL ||
        idx >= l->length)
    {
        return NULL;
    }

    struct t_list_node *current;

    // Traverse from the closer end
    if (idx > l->length / 2) {
        // Traverse backward from tail
        current = l->tail;
        unsigned steps_from_end = l->length - 1 - idx;
        for (unsigned i = 0; i < steps_from_end; i++) {
            current = current->prev;
        }
    } else {
        // Traverse forward from head
        current = l->head;
        for (unsigned i = 0; i < idx; i++) {
            current = current->next;
        }
    }
    return current;
}

/**
 * Moves a node in the list to another position.
 * @param l list
 * @param from from pos
 * @param to to pos
 * @return true on success, else false
 */
bool list_move_node(struct t_list *l, unsigned from, unsigned to) {
    if (l == NULL || from >= l->length || to >= l->length || from == to) {
        return false;
    }

    struct t_list_node *old = list_node_at(l, from);
    struct t_list_node *new = list_node_at(l, to);
    
    if (!old || !new) {
        return false;
    }

    // Extract the node
    struct t_list_node *extracted = list_node_extract(l, old);

    // Insert before 'new' or after, depending on direction
    struct t_list_node *insert_after = from < to
        ? new
        : new->prev;
    
    if (insert_after != NULL) {
        extracted->prev = insert_after;
        extracted->next = insert_after->next;
        if (insert_after->next) {
            insert_after->next->prev = extracted;
        }
        insert_after->next = extracted;
    } else {
        // Insert at head
        extracted->prev = NULL;
        extracted->next = l->head;
        if (l->head != NULL) {
            l->head->prev = extracted;
        }
        l->head = extracted;
    }

    if (extracted->next == NULL) {
        l->tail = extracted;
    }
    l->length++;
    return true;
}

/**
 * Creates and appends a node to the end of the list.
 * @param l list
 * @param key key value
 * @param value_i int64_t value
 * @param value_p sds value
 * @param user_data pointer to user_data
 * @return true on success, else false
 */
bool list_push(struct t_list *l, const char *key, int64_t value_i,
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
 * @param value_i int64_t value
 * @param value_p sds value
 * @param value_len value_p length
 * @param user_data pointer to user_data
 * @return true on success, else false
 */
bool list_push_len(struct t_list *l, const char *key, size_t key_len, int64_t value_i,
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
        n->prev = NULL;
        l->head = n;
    }
    else {
        //append to the list
        n->prev = l->tail;
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
 * @param value_i int64_t value
 * @param value_p sds value
 * @param user_data pointer to user_data
 * @return true on success, else false
 */
bool list_insert(struct t_list *l, const char *key, int64_t value_i,
        const char *value_p, void *user_data)
{
    //create new node
    struct t_list_node *n = malloc_assert(sizeof(struct t_list_node));
    n->key = sdsnew(key);
    n->value_i = value_i;
    n->value_p = value_p != NULL ? sdsnew(value_p) : NULL;
    n->user_data = user_data;
    n->prev = NULL;

    //switch head pointer
    n->next = l->head;
    if (l->head != NULL) {
        l->head->prev = n;
    }
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
 * @param value_i new int64_t value
 * @param value_p new sds value
 * @param user_data new user_data pointer
 * @return true on success, else false
 */
bool list_replace(struct t_list *l, unsigned idx, const char *key, int64_t value_i,
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
 * @param value_i new int64_t value
 * @param value_p new sds value
 * @param value_len new sds value len
 * @param user_data new user_data pointer
 * @return true on success, else false
 */
bool list_replace_len(struct t_list *l, unsigned idx, const char *key, size_t key_len, int64_t value_i,
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
 * @param value_i new int64_t value
 * @param value_p new sds value
 * @param user_data new user_data pointer
 * @param free_cb callback function to free old user_data pointer
 * @return true on success, else false
 */
bool list_replace_user_data(struct t_list *l, unsigned idx, const char *key, int64_t value_i,
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
 * @param value_i new int64_t value
 * @param value_p new sds value
 * @param value_len new sds value len
 * @param user_data new user_data pointer
 * @param free_cb callback function to free old user_data pointer
 * @return true on success, else false
 */
bool list_replace_len_user_data(struct t_list *l, unsigned idx, const char *key, size_t key_len, int64_t value_i,
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
 * @return NULL
 */
void *list_node_free(struct t_list_node *n) {
    list_node_free_user_data(n, list_free_cb_ignore_user_data);
    return NULL;
}

/**
 * Frees a list node and its user_data pointer
 * @param n node to free
 * @param free_cb callback function to free user_data pointer
 * @return NULL
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
bool list_remove_node(struct t_list *l, unsigned idx) {
    return list_remove_node_user_data(l, idx, list_free_cb_ignore_user_data);
}

/**
 * Removes the node at idx from the list and frees it and frees the user_data pointer
 * @param l list
 * @param idx node index to free
 * @param free_cb callback function to free user_data pointer
 * @return true on success, else false
 */
bool list_remove_node_user_data(struct t_list *l, unsigned idx, user_data_callback free_cb) {
    struct t_list_node *extracted = list_node_extract_at(l, idx);
    if (extracted == NULL) {
        return false;
    }
    list_node_free_user_data(extracted, free_cb);
    return true;
}

/**
 * Removes the node with key
 * Ignores user_data pointer
 * @param l list
 * @param key key
 * @return bool true on success, else false
 */
bool list_remove_node_by_key(struct t_list *l, const char *key) {
    return list_remove_node_by_key_user_data(l, key, NULL);
}

/**
 * Removes the node with key
 * @param l list
 * @param key key
 * @param free_cb Callback to free the user data
 * @return bool true on success, else false
 */
bool list_remove_node_by_key_user_data(struct t_list *l, const char *key, user_data_callback free_cb) {
    struct t_list_node *current = l->head;
    size_t key_len = strlen(key);

    while (current != NULL) {
        if (sdslen(current->key) == key_len &&
            strncmp(current->key, key, key_len) == 0)
        {
            struct t_list_node *extracted = list_node_extract(l, current);
            if (extracted != NULL) {
                list_node_free_user_data(extracted, free_cb);
                return true;
            }
            return false;
        }
        current = current->next;
    }
    return false;
}

/**
 * Removes the first node from the list and returns it.
 * This is only a shortcut for list_node_extract.
 * @param l list
 * @return pointer to list node
 */
struct t_list_node *list_shift_first(struct t_list *l) {
    return list_node_extract(l, l->head);
}

/**
 * Removes the node at idx from the list and returns it.
 * @param l list
 * @param idx node index to remove
 * @return pointer to list node
 */
struct t_list_node *list_node_extract_at(struct t_list *l, unsigned idx) {
    if (l->head == NULL ||
        idx >= l->length)
    {
        return NULL;
    }

    //get the node at idx and the previous node
    struct t_list_node *current = list_node_at(l, idx);
    return list_node_extract(l, current);
}

/**
 * Removes a node directly from the list
 * @param l list
 * @param node node to remove
 * @return pointer to extracted node
 */
struct t_list_node *list_node_extract(struct t_list *l, struct t_list_node *node) {
    if (node == NULL) {
        return NULL;
    }

    struct t_list_node *previous = node->prev;

    if (previous == NULL) {
        // Fix head
        l->head = node->next;
    }
    else {
        // Fix previous nodes next to skip over the removed node
        previous->next = node->next;
    }

    // Fix next node's prev pointer
    if (node->next != NULL) {
        node->next->prev = previous;
    }

    //Fix tail
    if (l->tail == node) {
        l->tail = previous;
    }
    l->length--;

    // Null out this node's pointers since it's not part of a list anymore
    node->next = NULL;
    node->prev = NULL;

    // Return the node
    return node;
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
        buffer = node_to_line_cb(buffer, current, true);
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
void list_crop(struct t_list *l, unsigned length, user_data_callback free_cb) {
    if (l->length <= length) {
        return;
    }
    if (length == 0) {
        list_clear(l);
        return;
    }
    // Find the new last node
    unsigned idx = length - 1;
    struct t_list_node *last = list_node_at(l, idx);

    // Free all nodes after the new last
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
