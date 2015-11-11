#ifndef RBKIT_STACK_FRAME
#define RBKIT_STACK_FRAME
#include <stddef.h>

typedef struct _rbkit_frame_data {
  const char * method_name;
  const char * label;
  const char * file;
  unsigned long line;
  int is_singleton_method;
  unsigned long thread_id;
} rbkit_frame_data;

typedef struct _rbkit_stack_trace {
  size_t frame_count;
  rbkit_frame_data *frames;
} rbkit_stack_trace;

void collect_stack_trace(rbkit_stack_trace *stacktrace);
void delete_stack_trace(rbkit_stack_trace *stacktrace);

#endif
