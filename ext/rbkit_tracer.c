//
//  rbkit.c
//  gather_stats
//
//  Created by Hemant Kumar on 26/04/14.
//  Copyright (c) 2014 Codemancers. All rights reserved.
//

#include "ruby/ruby.h"
#include "ruby/debug.h"
#include <stdio.h>
#include <assert.h>
#include "zmq.h"
#include "msgpack.h"

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

static const char *event_names[] = {
  "gc_start",
  "gc_end_m",
  "gc_end_s",
  "obj_created",
  "obj_destroyed"
};

static struct gc_hooks *logger;
static int tmp_keep_remains;
static void *zmq_publisher;
static void *zmq_context;

static void send_event(int event_index) {
  const char *event = event_names[event_index];
  msgpack_sbuffer_clear(logger->sbuf);
  msgpack_pack_raw(logger->msgpacker, strlen(event));
  msgpack_pack_raw_body(logger->msgpacker, event, strlen(event));
  zmq_send(zmq_publisher, logger->sbuf->data, logger->sbuf->size, 0);
}

static void trace_gc_invocation(void *data, int event_index) {
  if (event_index == 0) {
    send_event(1);
  } else if (event_index == 2) {
    send_event(2);
  }
}

static struct gc_hooks * get_trace_logger() {
  int i = 0;
  if (logger == 0) {
    logger = ALLOC_N(struct gc_hooks, 1);
    logger->enabled = Qfalse;
    logger->newobj_trace = 0;
    logger->freeobj_trace = 0;
    logger->keep_remains = tmp_keep_remains;
    logger->object_table = st_init_numtable();
    logger->str_table = st_init_strtable();

    for (i = 0; i < 3; i++) {
      logger->funcs[i] = trace_gc_invocation;
      logger->args[i] = (void *)event_names[i];
    }
    logger->sbuf = msgpack_sbuffer_new();
    logger->msgpacker = msgpack_packer_new(logger->sbuf, msgpack_sbuffer_write);
    logger->data = 0;
  }
  return logger;
}


static void
gc_start_i(VALUE tpval, void *data)
{
  struct gc_hooks *hooks = (struct gc_hooks *)data;
  (*hooks->funcs[0])(hooks->args[0], 0);
}

static void
gc_end_mark_i(VALUE tpval, void *data)
{
  struct gc_hooks *hooks = (struct gc_hooks *)data;
  (*hooks->funcs[1])(hooks->args[1], 1);
}

static void
gc_end_sweep_i(VALUE tpval, void *data)
{
  struct gc_hooks *hooks = (struct gc_hooks *)data;
  (*hooks->funcs[2])(hooks->args[2], 2);
}

static void
create_gc_hooks(void)
{
  int i;
  logger->hooks[0] =
    rb_tracepoint_new(0, RUBY_INTERNAL_EVENT_GC_START,     gc_start_i,     logger);
  logger->hooks[1] =
    rb_tracepoint_new(0, RUBY_INTERNAL_EVENT_GC_END_MARK,  gc_end_mark_i,  logger);
  logger->hooks[2] =
    rb_tracepoint_new(0, RUBY_INTERNAL_EVENT_GC_END_SWEEP, gc_end_sweep_i, logger);
  /* mark for GC */
  for (i=0; i<3; i++) rb_gc_register_mark_object(logger->hooks[i]);
}

static void newobj_i(VALUE tpval, void *data) {
  send_event(3);
}

static void freeobj_i(VALUE tpval, void *data) {
  send_event(4);
}

static VALUE start_stat_server() {
  logger = get_trace_logger();
  zmq_context = zmq_ctx_new();
  zmq_publisher = zmq_socket(zmq_context, ZMQ_PUB);
  int bind_result = zmq_bind(zmq_publisher, "tcp://*:5555");
  assert(bind_result == 0);

  logger->newobj_trace = rb_tracepoint_new(0, RUBY_INTERNAL_EVENT_NEWOBJ, newobj_i, logger);
  logger->freeobj_trace = rb_tracepoint_new(0, RUBY_INTERNAL_EVENT_FREEOBJ, freeobj_i, logger);
  create_gc_hooks();
  return Qnil;
}

static VALUE stop_stat_tracing() {
  if (logger->hooks[0] != 0) {
    rb_tracepoint_disable(logger->hooks[0]);
    rb_tracepoint_disable(logger->hooks[1]);
    rb_tracepoint_disable(logger->hooks[2]);
  }

  if (logger->newobj_trace) {
    rb_tracepoint_disable(logger->newobj_trace);
    rb_tracepoint_disable(logger->freeobj_trace);
  }
  logger->enabled = Qfalse;
  return Qnil;
}

static VALUE stop_stat_server() {
  if (logger->enabled == Qtrue)
    stop_stat_tracing();

  msgpack_sbuffer_destroy(logger->sbuf);
  msgpack_packer_free(logger->msgpacker);
  zmq_close(zmq_publisher);
  zmq_ctx_destroy(zmq_context);
  free(logger);
  return Qnil;
}

static VALUE start_stat_tracing() {
  rb_tracepoint_enable(logger->newobj_trace);
  rb_tracepoint_enable(logger->freeobj_trace);
  int i = 0;
  for (i=0; i<3; i++) {
    rb_tracepoint_enable(logger->hooks[i]);
  }
  logger->enabled = Qtrue;
  return Qnil;
}

void Init_rbkit_tracer(void) {
  VALUE objectStatsModule = rb_define_module("Rbkit");
  rb_define_module_function(objectStatsModule, "start_server", start_stat_server, 0);
  rb_define_module_function(objectStatsModule, "stop_server", stop_stat_server, 0);
  rb_define_module_function(objectStatsModule, "start_stat_tracing", start_stat_tracing, 0);
  rb_define_module_function(objectStatsModule, "stop_stat_tracing", stop_stat_tracing, 0);
}
