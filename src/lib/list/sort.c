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
#include "src/lib/mem.h"
#include "src/lib/utf8_wrapper.h"

// Private definitions

/**
 * Helper structure for bottom-up merge sort
 */

struct t_merge_result {
    struct t_list_node *head;  //!< Head pointer
    struct t_list_node *tail;  //!< Tail pointer
};

static bool sort_cb_value_i(struct t_list_node *first, struct t_list_node *second, enum list_sort_direction direction);
static bool sort_cb_value_p(struct t_list_node *first, struct t_list_node *second, enum list_sort_direction direction);
static bool sort_cb_key(struct t_list_node *first, struct t_list_node *second, enum list_sort_direction direction);
static void merge(struct t_list_node *first, struct t_list_node *first_end,
        struct t_list_node *second, struct t_list_node *second_end,
        enum list_sort_direction direction, list_sort_callback sort_cb,
        struct t_merge_result *result);

// Public functions

/**
 * The list sorting function. Uses bottom-up merge sort algorithm.
 * @param l pointer to list to sort
 * @param direction sort direction
 * @param sort_cb compare function
 * @return Always true
 */
bool list_sort_by_callback(struct t_list *l, enum list_sort_direction direction, list_sort_callback sort_cb) {
    if (l->length < 2) {
        return true;
    }

    struct t_list_node *head = l->head;
    struct t_list_node *tail = l->tail;

    // Pre-allocate result struct once for reuse
    struct t_merge_result *merged_result = malloc_assert(sizeof(struct t_merge_result));

    // Bottom-up merge sort: start with segment_size = 1, double each iteration
    for (unsigned segment_size = 1; segment_size < l->length; segment_size *= 2) {
        struct t_list_node *current = head;
        struct t_list_node *new_head = NULL;
        struct t_list_node *merged_tail = NULL;

        while (current != NULL) {
            // Get first segment of size segment_size
            struct t_list_node *first = current;
            struct t_list_node *first_end = first;
            unsigned count = 1;
            while (count < segment_size &&
                first_end->next != NULL)
            {
                first_end = first_end->next;
                count++;
            }

            // Get second segment of size segment_size
            struct t_list_node *second = first_end->next;
            struct t_list_node *second_end = second;
            if (second != NULL) {
                count = 1;
                while (count < segment_size &&
                    second_end->next != NULL)
                {
                    second_end = second_end->next;
                    count++;
                }
            }

            // Save next segment start
            struct t_list_node *next_segment = second_end != NULL
                ? second_end->next
                : NULL;

            // Break links to isolate runs
            first_end->next = NULL;
            if (second != NULL) {
                second->prev = NULL;
                second_end->next = NULL;
            }

            // Skip merge if only one run exists
            if (second == NULL) {
                // Only first run - no merge needed
                merged_result->head = first;
                merged_result->tail = first_end;
            }
            else {
                // Merge the two runs
                merge(first, first_end, second, second_end, direction, sort_cb, merged_result);
            }

            // Link merged result to overall result
            if (new_head == NULL) {
                new_head = merged_result->head;
                merged_result->head->prev = NULL;
            }
            else {
                merged_tail->next = merged_result->head;
                merged_result->head->prev = merged_tail;
            }

            merged_tail = merged_result->tail;
            current = next_segment;
        }

        head = new_head;
        tail = merged_tail;
    }

    l->head = head;
    l->tail = tail;

    free(merged_result);
    return true;
}

/**
 * Sorts the list by value_i. It uses the merge sort algorithm.
 * @param l pointer to list to sort
 * @param direction sort direction
 * @return Always true
 */
bool list_sort_by_value_i(struct t_list *l, enum list_sort_direction direction) {
    return list_sort_by_callback(l, direction, sort_cb_value_i);
}

/**
 * Sorts the list by value_p. It uses the merge sort algorithm.
 * @param l pointer to list to sort
 * @param direction sort direction
 * @return Always true
 */
bool list_sort_by_value_p(struct t_list *l, enum list_sort_direction direction) {
    return list_sort_by_callback(l, direction, sort_cb_value_p);
}

/**
 * Sorts the list by key. It uses the merge sort algorithm.
 * @param l pointer to list to sort
 * @param direction sort direction
 * @return Always true
 */
bool list_sort_by_key(struct t_list *l, enum list_sort_direction direction) {
    return list_sort_by_callback(l, direction, sort_cb_key);
}

// Internal functions

/**
 * Compare callback to sort by value_i
 * @param first first list node
 * @param second second list node
 * @param direction sort direction
 * @return true if first is greater than second
 */
static bool sort_cb_value_i(struct t_list_node *first, struct t_list_node *second, enum list_sort_direction direction) {
    if ((direction == LIST_SORT_ASC && first->value_i > second->value_i) ||
        (direction == LIST_SORT_DESC && first->value_i < second->value_i))
    {
        return true;
    }
    return false;
}

/**
 * Compare callback to sort by value_p
 * @param first first list node
 * @param second second list node
 * @param direction sort direction
 * @return true if first is greater than second
 */
static bool sort_cb_value_p(struct t_list_node *first, struct t_list_node *second, enum list_sort_direction direction) {
    int result = utf8_wrap_casecmp(first->value_p, sdslen(first->value_p), second->value_p, sdslen(second->value_p));
    if ((direction == LIST_SORT_ASC && result > 0) ||
        (direction == LIST_SORT_DESC && result < 0))
    {
        return true;
    }
    return false;
}

/**
 * Compare callback to sort by key
 * @param first first list node
 * @param second second list node
 * @param direction sort direction
 * @return true if first is greater than second
 */
static bool sort_cb_key(struct t_list_node *first, struct t_list_node *second, enum list_sort_direction direction) {
    int result = utf8_wrap_casecmp(first->key, sdslen(first->key), second->key, sdslen(second->key));
    if ((direction == LIST_SORT_ASC && result > 0) ||
        (direction == LIST_SORT_DESC && result < 0))
    {
        return true;
    }
    return false;
}

/**
 * Function to merge two sorted linked lists (ITERATIVE)
 * @param first Head pointer to first list
 * @param first_end Tail pointer to first list
 * @param second Head pointer to second list
 * @param second_end Tail pointer to second list
 * @param direction Sort direction
 * @param sort_cb Sort callback
 * @param result Merge result
 */
static void merge(struct t_list_node *first, struct t_list_node *first_end,
        struct t_list_node *second, struct t_list_node *second_end,
        enum list_sort_direction direction, list_sort_callback sort_cb,
        struct t_merge_result *result)
{
    if (first == NULL) {
        result->head = second;
        result->tail = second_end;
        return;
    }
    if (second == NULL) {
        result->head = first;
        result->tail = first_end;
        return;
    }

    // Use stack-allocated dummy to simplify linking
    struct t_list_node dummy;
    dummy.next = NULL;
    dummy.prev = NULL;

    // The dummy node is a sentinel that simplifies the merge logic
    // by eliminating special-case handling for the first node.
    struct t_list_node *current = &dummy;

    // Iterative merge - single pass, no recursion overhead
    while (first != NULL &&
           second != NULL)
    {
        if (sort_cb(first, second, direction) == false) {
            // first <= second, take from first
            current->next = first;
            first->prev = current;
            first = first->next;
        }
        else {
            // second < first, take from second
            current->next = second;
            second->prev = current;
            second = second->next;
        }
        current = current->next;
    }

    // Attach remaining elements (at most one list has remaining)
    if (first != NULL) {
        current->next = first;
        first->prev = current;
        current = first_end;
    }
    else if (second != NULL) {
        current->next = second;
        second->prev = current;
        current = second_end;
    }

    // Set results (skip dummy node)
    result->head = dummy.next;
    if (result->head != NULL) {
        result->head->prev = NULL;   // Head has no previous
        result->tail = current;      // current is now at tail
    }
}
