/*
 SPDX-License-Identifier: GPL-2.0-or-later
 myMPD (c) 2018-2020 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#ifndef __LIST_H__
#define __LIST_H__

struct node {
    sds data;
    sds extra;
    int value;
    struct node *next;
};

struct list {
    int length;
    struct node *list;
};

int list_init(struct list *l);
int list_push(struct list *l, const char *data, int value, const char *extra);
int list_insert(struct list *l, const char *data, int value, const char *extra);
int list_shift(struct list *l, unsigned idx);
struct node *list_node_extract(struct list *l, unsigned idx);
int list_replace(struct list *l, int pos, const char *data, int value, const char *extra);
int list_free(struct list *l);
int list_get_value(const struct list *l, const char *data);
sds list_get_extra(const struct list *l, const char *data);
struct node *list_get_node(const struct list *l, const char *data);
int list_shuffle(struct list *l);
int list_sort_by_value(struct list *l, bool order);
int list_swap_item(struct node *n1, struct node *n2);
struct node *list_node_at(const struct list * l, unsigned index);
#endif
