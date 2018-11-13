struct node {
    char *data;
    long value;
    struct node *next;
};


struct list {
    unsigned length;
    struct node *list;
};


int list_init(struct list *l);
int list_push(struct list *l, const char *data, int value);
int list_insert(struct list *l, const char *data, int value);
int list_shift(struct list *l, unsigned idx);
struct node *list_node_extract(struct list *l, unsigned idx);
int list_replace(struct list *l, int pos, const char *data, int value);
int list_free(struct list *l);
int list_get_value(const struct list *l, const char *data);
int list_shuffle(struct list *l);
int list_sort_by_value(struct list *l, bool order);
int list_swap_item(struct node *n1, struct node *n2);
struct node *list_node_at(const struct list * l, unsigned index);
