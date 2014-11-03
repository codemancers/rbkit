#ifndef RBKIT_ALLOCATION_INFO
#define RBKIT_ALLOCATION_INFO
#include <ruby.h>
#include "ruby/debug.h"

typedef struct _rbkit_allocation_info {
  const char *path;
  unsigned long line;
  const char *class_path;
  VALUE method_id;
  size_t generation;
} rbkit_allocation_info;

rbkit_allocation_info * new_rbkit_allocation_info(rb_trace_arg_t *tparg, st_table *str_table, st_table *object_table);
void delete_rbkit_allocation_info(rb_trace_arg_t *tparg, VALUE obj, st_table *str_table, st_table *object_table);

#endif
