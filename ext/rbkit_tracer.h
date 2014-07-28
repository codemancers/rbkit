#ifndef RBKIT_TRACER_H
#define RBKIT_TRACER_H

#include "ruby/ruby.h"
#include "ruby/debug.h"
#include <stdio.h>
#include <assert.h>
#include "zmq.h"
#include "msgpack.h"

struct  obj_allocation_info
{
  int living;
  VALUE flags;
  VALUE klass;

  /* allocation info */
  const char *path;
  unsigned long line;
  const char *class_path;
  VALUE mid;
  size_t generation;
};

// Structure is used to store profiling data
struct gc_hooks {
  VALUE hooks[3];
  VALUE enabled;
  void (*funcs[3])(void *data, int event_index);
  void *args[3];
  void *data;
  st_table *object_table;
  st_table *str_table;
  VALUE newobj_trace;
  VALUE freeobj_trace;
  int keep_remains;
  msgpack_sbuffer *sbuf;
  msgpack_packer *msgpacker;
};

char * tracer_string_recv(void *socket);
int tracer_string_send(void *socket, const char *message);
void pack_value_object(msgpack_packer *packer, VALUE value);
void pack_string(msgpack_packer *packer, char *string);
void pack_timestamp(msgpack_packer *packer);
void pack_event_header(msgpack_packer *packer, const char *event_type);
void pack_pointer(msgpack_packer *packer, VALUE object_id);


#endif
