#ifndef RBKIT_LIST_H
#define RBKIT_LIST_H

/*
 * A reusable linked list implementation.
 */

typedef struct _rbkit_node_t {
    struct _rbkit_node_t *next;
    void *item;
} rbkit_node_t;

typedef struct _rbkit_list_t {
    rbkit_node_t *head;               //  First item in list, if any
    rbkit_node_t *tail;               //  Last item in list, if any
    rbkit_node_t *cursor;             //  Current cursors for iteration
    size_t size;                //  Number of items in list
} rbkit_list_t;

rbkit_list_t * rbkit_list_new (void);
void * rbkit_list_first (rbkit_list_t *self_p);
void * rbkit_list_next (rbkit_list_t *self_p);
int rbkit_list_append (rbkit_list_t *self_p, void *item);
size_t rbkit_list_size (rbkit_list_t *self_p);
void rbkit_list_destroy (rbkit_list_t **self_p);
void rbkit_list_clear (rbkit_list_t *self_p);

#endif
