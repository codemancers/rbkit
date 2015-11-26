#ifndef RBKIT_OBJECT_TRACER
#define RBKIT_OBJECT_TRACER

#include "rbkit_stack_trace.h"
#include "rbkit_map.h"

#define MAX_NEW_OBJ_INFOS 1000

typedef struct _rbkit_new_object_info {
  unsigned long long object_id;
  const char *klass;
  const char *file;
  unsigned long line;
  size_t size;
  double timestamp;
  rbkit_stack_trace stacktrace;
} rbkit_new_object_info;

typedef struct _object_allocation_details {
  size_t count;
  rbkit_map_t *stacktraces_counts_map;
  //rbkit_stack_trace **stacktraces;
} rbkit_object_allocation_details;

void init_object_tracer();
void init_object_allocation_table();
void add_new_object_info(rbkit_new_object_info *info);
rbkit_map_t *get_allocation_map();
int watch_object_source(const char *file, const char *object_detail);

#endif
