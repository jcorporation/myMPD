#include <stdlib.h>
#include <string.h>
#include "list.h"

int list_init(struct list *list) {
    list->length = 0;
    list->list = NULL;
    return 0;
}

int list_push(struct list *l, char *data, int value) {
    struct node *n = malloc(sizeof(struct node));
    n->value = value;
    n->data = malloc(strlen(data));
    n->data = strdup(data);
    n->next = NULL;

    struct node **next = &l->list;
    while (*next != NULL) {
        next = &(*next)->next;
    }
    *next = n;
    l->length++;
    return 0;
}

int list_free(struct list *l) {
    struct node * current = l->list, *tmp = NULL;
    while (current != NULL) {
        free(current->data);
        tmp = current;
        current = current->next;
        free(tmp);
    }
    list_init(l);
    return 0;
}
