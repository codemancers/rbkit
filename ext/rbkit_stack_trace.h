#ifndef RBKIT_STACK_FRAME
#define RBKIT_STACK_FRAME
#include <stddef.h>
#include "rbkit_map.h"

typedef struct _rbkit_frame_data {
  char * method_name;
  char * label;
  char * file;
  unsigned long line;
  int is_singleton_method;
  //unsigned long thread_id;
} rbkit_frame_data;

typedef struct _rbkit_stack_trace {
  size_t frame_count;
  rbkit_frame_data *frames;
} rbkit_stack_trace;

void init_stack_trace_maps();
rbkit_stack_trace *collect_stack_trace();
rbkit_map_t *get_stack_traces();
void clear_stack_traces();
void delete_stack_trace(rbkit_stack_trace *stacktrace);

#endif
