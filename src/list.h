/*
 SPDX-License-Identifier: GPL-2.0-or-later
 myMPD (c) 2018-2020 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#ifndef __LIST_H__
#define __LIST_H__

struct list_node {
    sds key;
    sds value_p;
    long value_i;
    void *user_data;
    struct list_node *next;
};

struct list {
    int length;
    struct list_node *head;
    struct list_node *tail;
};

bool list_init(struct list *l);
bool list_push(struct list *l, const char *key, long value_i, const char *value_p, void *user_data);
bool list_insert(struct list *l, const char *key, long value_i, const char *value_p, void *user_data);
bool list_shift(struct list *l, unsigned idx);
bool list_replace(struct list *l, int pos, const char *key, long value_i, const char *value_p, void *user_data);
bool list_free(struct list *l);
long list_get_value_i(const struct list *l, const char *key);
sds list_get_value_p(const struct list *l, const char *key);
void *list_get_user_data(const struct list *l, const char *key);
struct list_node *list_get_node(const struct list *l, const char *key);
bool list_shuffle(struct list *l);
bool list_sort_by_value_i(struct list *l, bool order);
bool list_sort_by_value_p(struct list *l, bool order);
bool list_sort_by_key(struct list *l, bool order);
bool list_swap_item(struct list_node *n1, struct list_node *n2);
struct list_node *list_node_at(const struct list * l, unsigned index);
#endif
