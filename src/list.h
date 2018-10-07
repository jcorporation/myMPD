struct node {
    char *data;
    int value;
    struct node *next;
};

struct list {
    unsigned length;
    struct node *list;
};


int list_init(struct list *);
int list_push(struct list *l, char *data, int value);
int list_free(struct list *l);
