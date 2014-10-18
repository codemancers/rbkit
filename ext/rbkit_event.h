#ifndef RBKIT_EVENT
#define RBKIT_EVENT
#include "rbkit_allocation_info.h"

typedef enum _event_type {
  obj_created,
  obj_destroyed,
  gc_start,
  gc_end_m,
  gc_end_s,
  object_space_dump,
  gc_stats
} rbkit_event_type;

typedef struct _rbkit_event_header {
  rbkit_event_type event_type;
  double timestamp; //In milliseconds
} rbkit_event_header;

typedef struct _rbkit_obj_created_event {
  rbkit_event_header event_header;
  void *object_id;
  char *klass;
  rbkit_allocation_info *allocation_info;
} rbkit_obj_created_event;

rbkit_obj_created_event *new_rbkit_obj_created_event(void *object_id, char *klass, rbkit_allocation_info *info);

typedef struct _rbkit_obj_destroyed_event {
  rbkit_event_header event_header;
  void *object_id;
} rbkit_obj_destroyed_event;

rbkit_obj_destroyed_event *new_rbkit_obj_destroyed_event(void *object_id);

#endif
