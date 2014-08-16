#include <stdlib.h>
#include "list.h"

static void reset_list(list_t * self_p) {
  self_p->tail = NULL;
  self_p->head = NULL;
  self_p->cursor = NULL;
  self_p->size = 0;
}

list_t * list_new (void) {
  list_t *self_p = (list_t *) malloc (sizeof (list_t));
  reset_list(self_p);
  return self_p;
}

// Get the first item in the list
void * list_first (list_t *self_p)
{
  self_p->cursor = self_p->head;
  if (self_p->cursor)
    return self_p->cursor->item;
  else
    return NULL;
}

// Get next item in the list
void * list_next (list_t *self_p) {
  if (self_p->cursor)
    self_p->cursor = self_p->cursor->next;
  else
    self_p->cursor = self_p->head;
  if (self_p->cursor)
    return self_p->cursor->item;
  else
    return NULL;
}


//  Append an item to the end of the list, return 0 if OK
//  or -1 if this failed for some reason (out of memory).
int list_append (list_t *self_p, void *item) {
  if (!item)
    return -1;

  node_t *node;
  node = (node_t *) malloc (sizeof (node_t));
  if (!node)
    return -1;

  node->item = item;
  if (self_p->tail)
    self_p->tail->next = node;
  else
    self_p->head = node;

  self_p->tail = node;
  node->next = NULL;

  self_p->size++;
  self_p->cursor = NULL;
  return 0;
}

// Get the number of items in the list
size_t list_size (list_t *self_p) {
  return self_p->size;
}

static void delete_nodes(list_t *self_p) {
  node_t *node = (self_p)->head;
  while (node) {
    node_t *next = node->next;
    free (node);
    node = next;
  }
}

// Deletes the nodes contained in the list and
// resets head, next and cursor.
void list_clear (list_t *self_p) {
  delete_nodes(self_p);
  reset_list(self_p);
}

// Deletes the nodes contained in the list and
// deletes the list itself.
void list_destroy (list_t *self_p) {
  delete_nodes(self_p);
  free(self_p);
  self_p = NULL;
}
