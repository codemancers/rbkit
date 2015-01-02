#ifndef RBKIT_EVENT
#define RBKIT_EVENT
#include "rbkit_allocation_info.h"
#include "rbkit_object_graph.h"

typedef enum _event_type {
  obj_created,
  obj_destroyed,
  gc_start,
  gc_end_m,
  gc_end_s,
  object_space_dump,
  gc_stats,
  event_collection,
  handshake
} rbkit_event_type;

VALUE rbkit_event_types_as_hash();

typedef struct _rbkit_event_header {
  rbkit_event_type event_type;
} rbkit_event_header;

typedef struct _rbkit_obj_created_event {
  rbkit_event_header event_header;
  unsigned long long object_id;
  const char *klass;
  rbkit_allocation_info *allocation_info;
} rbkit_obj_created_event;

rbkit_obj_created_event *new_rbkit_obj_created_event(unsigned long long object_id, const char *klass, rbkit_allocation_info *info);

typedef struct _rbkit_obj_destroyed_event {
  rbkit_event_header event_header;
  unsigned long long object_id;
} rbkit_obj_destroyed_event;

rbkit_obj_destroyed_event *new_rbkit_obj_destroyed_event(unsigned long long object_id);

typedef struct _rbkit_hash_event {
  rbkit_event_header event_header;
  VALUE hash;
} rbkit_hash_event;

rbkit_hash_event *new_rbkit_hash_event(rbkit_event_type event_type, VALUE hash);

typedef struct _rbkit_object_space_dump_event {
  rbkit_event_header event_header;
  rbkit_object_dump *dump;
} rbkit_object_space_dump_event;

rbkit_object_space_dump_event *new_rbkit_object_space_dump_event(rbkit_object_dump *dump);

typedef struct _rbkit_event_collection_event {
  rbkit_event_header event_header;
  void *buffer;
  size_t buffer_size;
  size_t message_count;
} rbkit_event_collection_event;

rbkit_event_collection_event *new_rbkit_event_collection_event(void * buffer, size_t buffer_size, size_t message_count);

#endif
