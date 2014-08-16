#ifndef LIST_H
#define LIST_H

/*
 * A reusable linked list implementation.
 */

typedef struct _node_t {
    struct _node_t *next;
    void *item;
} node_t;

typedef struct _list_t {
    node_t *head;               //  First item in list, if any
    node_t *tail;               //  Last item in list, if any
    node_t *cursor;             //  Current cursors for iteration
    size_t size;                //  Number of items in list
} list_t;

list_t * list_new (void);
void * list_first (list_t *self_p);
void * list_next (list_t *self_p);
int list_append (list_t *self_p, void *item);
size_t list_size (list_t *self_p);
void list_destroy (list_t *self_p);
void list_clear (list_t *self_p);

#endif
