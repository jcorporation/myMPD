/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2023 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#ifndef MYMPD_LIST_H
#define MYMPD_LIST_H

#include "dist/sds/sds.h"

#include <stdbool.h>

/**
 * List node struct holding list key/value and additional data
 */
struct t_list_node {
    sds key;                   //!< key string
    sds value_p;               //!< string value
    long long value_i;         //!< long long value
    void *user_data;           //!< custom data
    struct t_list_node *next;  //!< pointer to next node
};

/**
 * List struct itself
 */
struct t_list {
    long length;               //!< length of the list
    struct t_list_node *head;  //!< pointer to first node
    struct t_list_node *tail;  //!< pointer to last node
};

typedef void (*user_data_callback) (struct t_list_node *current);
typedef sds (*list_node_to_line_callback) (sds buffer, struct t_list_node *current);

struct t_list *list_new(void);
void list_init(struct t_list *l);
void list_clear(struct t_list *l);
void *list_free(struct t_list *l);
void list_clear_user_data(struct t_list *l, user_data_callback free_cb);
void *list_free_user_data(struct t_list *l, user_data_callback free_cb);
void list_free_cb_ignore_user_data(struct t_list_node *current);
void list_free_cb_sds_user_data(struct t_list_node *current);
void list_free_cb_ptr_user_data(struct t_list_node *current);
void *list_node_free_user_data(struct t_list_node *n, user_data_callback free_cb);
void *list_node_free(struct t_list_node *n);

bool list_push(struct t_list *l, const char *key, long long value_i,
        const char *value_p, void *user_data);
bool list_push_len(struct t_list *l, const char *key, size_t key_len, long long value_i,
        const char *value_p, size_t value_len, void *user_data);
bool list_insert(struct t_list *l, const char *key, long long value_i,
        const char *value_p, void *user_data);

bool list_replace(struct t_list *l, long idx, const char *key, long long value_i,
        const char *value_p, void *user_data);
bool list_replace_len(struct t_list *l, long idx, const char *key, size_t key_len, long long value_i,
        const char *value_p, size_t value_len, void *user_data);
bool list_replace_user_data(struct t_list *l, long idx, const char *key, long long value_i,
        const char *value_p, void *user_data, user_data_callback free_cb);
bool list_replace_len_user_data(struct t_list *l, long idx, const char *key, size_t key_len, long long value_i,
        const char *value_p, size_t value_len, void *user_data, user_data_callback free_cb);

bool list_shuffle(struct t_list *l);
bool list_swap_item(struct t_list_node *n1, struct t_list_node *n2);
bool list_move_item_pos(struct t_list *l, long from, long to);

int list_get_node_idx(const struct t_list *l, const char *key);
struct t_list_node *list_get_node(const struct t_list *l, const char *key);
struct t_list_node *list_node_at(const struct t_list *l, long idx);
struct t_list_node *list_node_prev_at(const struct t_list *l, long idx, struct t_list_node **previous);
struct t_list_node *list_shift_first(struct t_list *l);
struct t_list_node *list_node_extract(struct t_list *l, long idx);

bool list_remove_node(struct t_list *l, long idx);
bool list_remove_node_user_data(struct t_list *l, long idx, user_data_callback free_cb);

bool list_write_to_disk(sds filepath, struct t_list *l, list_node_to_line_callback node_to_line_cb);
#endif
