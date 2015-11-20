#ifndef RBKIT_OBJECT_TRACER
#define RBKIT_OBJECT_TRACER

#include "rbkit_stack_trace.h"
#include "ruby/st.h"

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

typedef struct _rbkit_map {
  st_table *table;
  size_t count;
} rbkit_map_t;

void init_object_allocation_table();
void add_new_object_info(rbkit_new_object_info *info);
rbkit_map_t *get_allocation_map();

#endif
