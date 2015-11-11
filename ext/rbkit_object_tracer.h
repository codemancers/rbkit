#ifndef RBKIT_OBJECT_TRACER
#define RBKIT_OBJECT_TRACER

#include "rbkit_stack_trace.h"

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

typedef struct _object_infos {
  rbkit_new_object_info *info_list[MAX_NEW_OBJ_INFOS];
  size_t count;
} rbkit_object_allocation_infos;

void push_new_object_allocation_info(rbkit_new_object_info *info);
rbkit_object_allocation_infos *get_object_allocation_infos();
int object_allocation_info_full();
#endif
