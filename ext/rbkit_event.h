#ifndef RBKIT_EVENT
#define RBKIT_EVENT
#include "rbkit_allocation_info.h"
#include "rbkit_object_graph.h"
#include "rbkit_sampling_profiler.h"
#include "rbkit_object_tracer.h"

typedef enum _event_type {
  allocation_snapshot,
  gc_start,
  gc_end_m,
  gc_end_s,
  object_space_dump,
  gc_stats,
  event_collection,
  handshake,
  cpu_sample
} rbkit_event_type;

VALUE rbkit_event_types_as_hash();

typedef struct _rbkit_event_header {
  rbkit_event_type event_type;
} rbkit_event_header;

typedef struct _rbkit_hash_event {
  rbkit_event_header event_header;
  VALUE hash;
} rbkit_hash_event;

rbkit_hash_event *new_rbkit_hash_event(rbkit_event_type event_type, VALUE hash);

typedef struct _rbkit_object_space_dump_event {
  rbkit_event_header event_header;
  rbkit_object_dump *dump;
  size_t packed_objects;
  size_t object_count;
  rbkit_object_dump_page *current_page;
  size_t current_page_index;
  int correlation_id;
} rbkit_object_space_dump_event;

rbkit_object_space_dump_event *new_rbkit_object_space_dump_event(rbkit_object_dump *dump);

typedef struct _rbkit_cpu_sample_event {
  rbkit_event_header event_header;
  rbkit_cpu_sample *sample;
} rbkit_cpu_sample_event;

rbkit_cpu_sample_event *new_rbkit_cpu_sample_event(rbkit_cpu_sample *sample);

typedef struct _rbkit_event_collection_event {
  rbkit_event_header event_header;
  void *buffer;
  size_t buffer_size;
  size_t message_count;
} rbkit_event_collection_event;

rbkit_event_collection_event *new_rbkit_event_collection_event(void * buffer, size_t buffer_size, size_t message_count);

typedef struct _rbkit_allocation_snapshot_event {
  rbkit_event_header event_header;
  rbkit_map_t *allocation_map;
} rbkit_allocation_snapshot_event;

rbkit_allocation_snapshot_event *new_rbkit_allocation_snapshot_event(rbkit_map_t *allocation_map);

#endif
