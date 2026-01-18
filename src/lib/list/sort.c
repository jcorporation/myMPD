/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2026 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

/*! \file
 * \brief Linked list implementation
 */

#include "compile_time.h"
#include "src/lib/list/sort.h"

#include "dist/sds/sds.h"
#include "src/lib/list/list.h"
#include "src/lib/utf8_wrapper.h"

// Private definitions
static bool sort_cmp_value_i(struct t_list_node *current, struct t_list_node *next, enum list_sort_direction direction);
static bool sort_cmp_value_p(struct t_list_node *current, struct t_list_node *next, enum list_sort_direction direction);
static bool sort_cmp_key(struct t_list_node *current, struct t_list_node *next, enum list_sort_direction direction);
static struct t_list_node* merge(struct t_list_node* first, struct t_list_node* second,
        enum list_sort_direction direction, list_sort_callback sort_cb);
static void split_list_half(struct t_list_node* source, struct t_list_node** front, struct t_list_node** back);
static void merge_sort(struct t_list_node** head_ref, enum list_sort_direction direction, list_sort_callback sort_cb);

// Public functions

/**
 * The list sorting function. It uses the merge sort algorithm.
 * @param l pointer to list to sort
 * @param direction sort direction
 * @param sort_cb compare function
 * @return true on success, else false
 */
bool list_sort_by_callback(struct t_list *l, enum list_sort_direction direction, list_sort_callback sort_cb) {
    merge_sort(&l->head, direction, sort_cb);
    // Fix tail
    struct t_list_node *current = l->head;
    while (current->next != NULL) {
        current = current->next;
    }
    l->tail = current;
    return true;
}

/**
 * Sorts the list by value_i. It uses the merge sort algorithm.
 * @param l pointer to list to sort
 * @param direction sort direction
 * @return true on success, else false
 */
bool list_sort_by_value_i(struct t_list *l, enum list_sort_direction direction) {
    return list_sort_by_callback(l, direction, sort_cmp_value_i);
}

/**
 * Sorts the list by value_p. It uses the merge sort algorithm.
 * @param l pointer to list to sort
 * @param direction sort direction
 * @return true on success, else false
 */
bool list_sort_by_value_p(struct t_list *l, enum list_sort_direction direction) {
    return list_sort_by_callback(l, direction, sort_cmp_value_p);
}

/**
 * Sorts the list by key. It uses the merge sort algorithm.
 * @param l pointer to list to sort
 * @param direction sort direction
 * @return true on success, else false
 */
bool list_sort_by_key(struct t_list *l, enum list_sort_direction direction) {
    return list_sort_by_callback(l, direction, sort_cmp_key);
}

// Internal functions

/**
 * Internal compare function to sort by value_i
 * @param current current list node
 * @param next next list node
 * @param direction sort direction
 * @return true if current is greater than next
 */
static bool sort_cmp_value_i(struct t_list_node *current, struct t_list_node *next, enum list_sort_direction direction) {
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
static bool sort_cmp_value_p(struct t_list_node *current, struct t_list_node *next, enum list_sort_direction direction) {
    int result = utf8_wrap_casecmp(current->value_p, sdslen(current->value_p), next->value_p, sdslen(next->value_p));
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
static bool sort_cmp_key(struct t_list_node *current, struct t_list_node *next, enum list_sort_direction direction) {
    int result = utf8_wrap_casecmp(current->key, sdslen(current->key), next->key, sdslen(next->key));
    if ((direction == LIST_SORT_ASC && result > 0) ||
        (direction == LIST_SORT_DESC && result < 0))
    {
        return true;
    }
    return false;
}

/**
 * Function to merge two sorted linked lists
 * @param first Head pointer to first list
 * @param second Head pointer to second list
 * @param direction Sort direction
 * @param sort_cb Sort callback
 * @return struct t_list_node* Head pointer to merged list
 */
static struct t_list_node *merge(struct t_list_node *first, struct t_list_node *second,
        enum list_sort_direction direction, list_sort_callback sort_cb)
{
    // Base cases
    if (first == NULL) {
        return second;
    }
    if (second == NULL) {
        return first;
    }

    struct t_list_node *result = NULL;
    if (sort_cb(first, second, direction) == false) {
        result = first;
        result->next = merge(first->next, second, direction, sort_cb);
    }
    else {
        result = second;
        result->next = merge(first, second->next, direction, sort_cb);
    }
    return result;
}

/**
 * Function to split the linked list into two halves
 * @param source Head pointer to list that should be split
 * @param front Head pointer for the first half
 * @param back  Head pointer for the second half
 */
static void split_list_half(struct t_list_node* source, struct t_list_node** front, struct t_list_node** back) {
    struct t_list_node *slow = source;
    struct t_list_node *fast = source->next;

    // Iterate through the linked list until the fast pointer reaches the end
    while (fast != NULL) {
        fast = fast->next;
        if (fast != NULL) {
            slow = slow->next;
            fast = fast->next;
        }
    }

    *front = source;     // First half
    *back = slow->next;  // Second half
    slow->next = NULL;   // Split the list into two halves
}

/**
 * Merge Sort function
 * @param headRef Reference to head pointer
 * @param direction Sort direction
 * @param sort_cb Sort callback
 */
static void merge_sort(struct t_list_node** head_ref, enum list_sort_direction direction, list_sort_callback sort_cb) {
    struct t_list_node *head = *head_ref;
    struct t_list_node *first_half;
    struct t_list_node *second_half;

    // Base case
    if (head == NULL ||
        head->next == NULL)
    {
        return;
    }

    // Split the list into first and second half sublists
    split_list_half(head, &first_half, &second_half);

    // Recursively sort the sublists
    merge_sort(&first_half, direction, sort_cb);
    merge_sort(&second_half, direction, sort_cb);

    // Merge the sorted lists
    *head_ref = merge(first_half, second_half, direction, sort_cb);
}
