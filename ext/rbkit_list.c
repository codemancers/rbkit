#include <stdlib.h>
#include "rbkit_list.h"

static void reset_list(rbkit_list_t * self_p) {
  if(!self_p)
    return;
  self_p->tail = NULL;
  self_p->head = NULL;
  self_p->cursor = NULL;
  self_p->size = 0;
}

rbkit_list_t * rbkit_list_new (void) {
  rbkit_list_t *self_p = (rbkit_list_t *) malloc (sizeof (rbkit_list_t));
  reset_list(self_p);
  return self_p;
}

// Get the first item in the list
void * rbkit_list_first (rbkit_list_t *self_p)
{
  if(!self_p)
    return NULL;
  self_p->cursor = self_p->head;
  if (self_p->cursor)
    return self_p->cursor->item;
  else
    return NULL;
}

// Get next item in the list
void * rbkit_list_next (rbkit_list_t *self_p) {
  if(!self_p)
    return NULL;
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
int rbkit_list_append (rbkit_list_t *self_p, void *item) {
  if(!self_p)
    return -1;
  if (!item)
    return -1;

  rbkit_node_t *node;
  node = (rbkit_node_t *) malloc (sizeof (rbkit_node_t));
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
size_t rbkit_list_size (rbkit_list_t *self_p) {
  if(!self_p)
    return 0;
  return self_p->size;
}

static void delete_nodes(rbkit_list_t *self_p) {
  if(!self_p)
    return;
  rbkit_node_t *node = (self_p)->head;
  while (node) {
    rbkit_node_t *next = node->next;
    free (node);
    node = next;
  }
}

// Deletes the nodes contained in the list and
// resets head, next and cursor.
void rbkit_list_clear (rbkit_list_t *self_p) {
  delete_nodes(self_p);
  reset_list(self_p);
}

// Deletes the nodes contained in the list and
// deletes the list itself.
void rbkit_list_destroy (rbkit_list_t **self_p) {
  if(!self_p || !(*self_p))
    return;
  rbkit_list_t *current = *self_p;
  delete_nodes(*self_p);
  free(current);
  *self_p = NULL;
}
