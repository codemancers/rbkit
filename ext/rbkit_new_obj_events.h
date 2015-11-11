#ifndef RBKIT_OBJ_CREATED_EVENTS
#define RBKIT_OBJ_CREATED_EVENTS
#include "rbkit_event.h"

#define  400

typedef struct _obj_created_events {
  size_t index;
  rbkit_obj_created_event events[1000];
} rbkit_obj_created_events;

#endif
