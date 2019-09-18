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

#ifndef __LIST_H__
#define __LIST_H__

struct node {
    sds data;
    void *extra;
    int value;
    struct node *next;
};

struct list {
    int length;
    struct node *list;
};

int list_init(struct list *l);
int list_push(struct list *l, const char *data, int value, void *extra);
int list_insert(struct list *l, const char *data, int value, void *extra);
int list_shift(struct list *l, unsigned idx);
struct node *list_node_extract(struct list *l, unsigned idx);
int list_replace(struct list *l, int pos, const char *data, int value, void *extra);
int list_free(struct list *l);
int list_get_value(const struct list *l, const char *data);
void *list_get_extra(const struct list *l, const char *data);
struct node *list_get_node(const struct list *l, const char *data);
int list_shuffle(struct list *l);
int list_sort_by_value(struct list *l, bool order);
int list_swap_item(struct node *n1, struct node *n2);
struct node *list_node_at(const struct list * l, unsigned index);
#endif
