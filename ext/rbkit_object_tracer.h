#ifndef RBKIT_OBJECT_TRACER
#define RBKIT_OBJECT_TRACER

#include "rbkit_stack_trace.h"
#include "rbkit_map.h"
#include "ruby/ruby.h"
#include "ruby/debug.h"

typedef struct _rbkit_object_signature {
  char *file;
  unsigned long line;
  char *klass;
  size_t size;
  rbkit_stack_trace *stacktrace;
} rbkit_object_signature;

rbkit_object_signature *find_or_create_object_signature(const unsigned long object_id,
    const char *file, const unsigned long line, const char *klass, const size_t size);
void delete_object(unsigned long object_id);
void init_object_tracer();

#endif
