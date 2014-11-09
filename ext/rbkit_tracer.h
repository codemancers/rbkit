#ifndef RBKIT_TRACER_H
#define RBKIT_TRACER_H

#include "ruby/ruby.h"
#include "ruby/debug.h"
#include <stdio.h>
#include <assert.h>
#include "zmq.h"
#include "msgpack.h"

// Structure is used to store profiling data
typedef struct _rbkit_logger {
  VALUE hooks[3];
  VALUE object_trace_enabled;
  VALUE execution_trace_enabled;
  void *data;
  st_table *object_table;
  st_table *str_table;
  VALUE newobj_trace;
  VALUE freeobj_trace;
  VALUE execution_trace;
  msgpack_sbuffer *sbuf;
  msgpack_packer *msgpacker;
} rbkit_logger;

char * tracer_string_recv(void *socket);
int tracer_string_send(void *socket, const char *message);

#endif
