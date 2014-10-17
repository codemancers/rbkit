#include "rbkit_event.h"

rbkit_obj_created_event *new_rbkit_obj_created_event(void *object_id,
    char *klass, rbkit_allocation_info *info) {
  rbkit_obj_created_event *event = malloc(sizeof(rbkit_obj_created_event));

  rbkit_event_header *header = event;
  header->event_type = obj_created;

  event->object_id = object_id;
  event->klass = klass;
  event->allocation_info = info;
  return event;
}

